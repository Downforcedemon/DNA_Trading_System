#include "core/IBKRConnection.hpp"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "Contract.h"
#include <iostream>
#include <thread>
#include <chrono>

IBKRConnection::IBKRConnection(IMarketDataListener* listener)
    : listener_(listener)
    , client_(nullptr)
    , nextOrderId_(0)
    , nextTickerId_(1) {
}

IBKRConnection::~IBKRConnection() {
    disconnect();
}

bool IBKRConnection::connect(const std::string& host, int port, int clientId) {
    // Create ONE signal shared by both EClientSocket and EReader
    signal_ = std::make_unique<EReaderOSSignal>(2000);  // 2s timeout
    client_ = std::make_unique<EClientSocket>(this, signal_.get());

    // Attempt connection
    bool connected = client_->eConnect(host.c_str(), port, clientId, false);

    if (connected) {
        // Start EReader thread — uses the SAME signal as EClientSocket
        reader_ = std::make_unique<EReader>(client_.get(), signal_.get());
        reader_->start();
        std::cout << "Connected to IB Gateway at " << host << ":" << port << std::endl;
    } else {
        std::cout << "Failed to connect to IB Gateway" << std::endl;
        return false;
    }

    return true;
}

void IBKRConnection::disconnect() {
    if (client_ && client_->isConnected()) {
        client_->eDisconnect();
        std::cout << "Disconnected from IB Gateway" << std::endl;
    }
}

bool IBKRConnection::isConnected() {
    return client_ && client_->isConnected();
}

void IBKRConnection::subscribeMarketData(const std::string& symbol) {
    if (!isConnected()) {
        std::cout << "Not connected to IB Gateway" << std::endl;
        return;
    }

    // Create contract for the symbol
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";

    int tickerId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tickerId = nextTickerId_;
        tickerIdToSymbol_[tickerId] = symbol;
        symbolToTickerId_[symbol] = tickerId;
        nextTickerId_++;
    }

    // Request market data (outside lock — this is a network call)
    client_->reqMktData(tickerId, contract, "", false, false, TagValueListSPtr());

    std::cout << "Subscribed to " << symbol << " (ID: " << tickerId << ")" << std::endl;
}

void IBKRConnection::unsubscribeMarketData(const std::string& symbol) {
    if (!isConnected()) return;

    int tickerId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = symbolToTickerId_.find(symbol);
        if (it == symbolToTickerId_.end()) return;  // not subscribed
        tickerId = it->second;
        symbolToTickerId_.erase(it);
        tickerIdToSymbol_.erase(tickerId);
    }

    client_->cancelMktData(tickerId);
    std::cout << "Unsubscribed from " << symbol << std::endl;
}

void IBKRConnection::processMessages() {
    if (signal_ && reader_) {
        // Wait for EReader to signal that messages are ready (with timeout)
        signal_->waitForSignal();
        // Dispatch all queued messages to our EWrapper callbacks
        reader_->processMsgs();
    }
}

// ========== IBKR Callbacks (called from EReader thread) ==========

void IBKRConnection::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {
    // field == 4 is LAST price (actual trade price)
    // field == 1 is BID
    // field == 2 is ASK

    if (field == 4) {  // LAST price
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tickerIdToSymbol_.find(tickerId);
        if (it != tickerIdToSymbol_.end()) {
            const std::string& symbol = it->second;
            listener_->onPriceUpdate(symbol, price, std::time(nullptr));
        }
    }
}

void IBKRConnection::tickSize(TickerId tickerId, TickType field, Decimal size) {
    // field == 5 is LAST size (volume of last trade)
    // field == 0 is BID size
    // field == 3 is ASK size
    if (field == 5) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tickerIdToSymbol_.find(tickerId);
        if (it != tickerIdToSymbol_.end()) {
            const std::string& symbol = it->second;
            listener_->onSizeUpdate(symbol, static_cast<int>(size), std::time(nullptr));
        }
    }
}

void IBKRConnection::error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) {
    std::cout << "Error [" << errorCode << "]: " << errorString << std::endl;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tickerIdToSymbol_.find(id);
    if (it != tickerIdToSymbol_.end()) {
        const std::string& symbol = it->second;
        listener_->onError(symbol, errorCode, errorString);
    }
}

void IBKRConnection::nextValidId(OrderId orderId) {
    nextOrderId_ = orderId;
    std::cout << "Connection ready. Next Order ID: " << orderId << std::endl;
}

void IBKRConnection::connectionClosed() {
    std::cout << "Connection closed by IB Gateway" << std::endl;
}

