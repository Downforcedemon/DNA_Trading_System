#include "core/IBKRConnection.hpp"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "Contract.h"
#include <iostream>
#include <thread>
#include <chrono>

IBKRConnection::IBKRConnection(SymbolManager& manager)
    : symbolManager_(manager)
    , client_(nullptr)
    , nextOrderId_(0)
    , nextTickerId_(1) {
}

IBKRConnection::~IBKRConnection() {
    disconnect();
}

bool IBKRConnection::connect(const std::string& host, int port, int clientId) {
    // Create the client socket
    client_ = std::make_unique<EClientSocket>(this, new EReaderOSSignal());
    
    // Attempt connection
    bool connected = client_->eConnect(host.c_str(), port, clientId, false);
    
    if (connected) {
        std::cout << "✅ Connected to IB Gateway at " << host << ":" << port << std::endl;
    } else {
        std::cout << "❌ Failed to connect to IB Gateway" << std::endl;
        return false;
    }
    
    return true;
}

void IBKRConnection::disconnect() {
    if (client_ && client_->isConnected()) {
        client_->eDisconnect();
        std::cout << "🔌 Disconnected from IB Gateway" << std::endl;
    }
}

bool IBKRConnection::isConnected() {
    return client_ && client_->isConnected();
}

void IBKRConnection::subscribeMarketData(const std::string& symbol) {
    if (!isConnected()) {
        std::cout << "⚠️  Not connected to IB Gateway" << std::endl;
        return;
    }
    
    // Create contract for the symbol
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";
    
    // Request market data
    client_->reqMktData(nextTickerId_, contract, "", false, false, TagValueListSPtr());
   
    // Store the mapping to identify price updates later
    tickerIdToSymbol_[nextTickerId_] = symbol; 

    std::cout << "📊 Subscribed to market data for " << symbol << " (ID: " << nextTickerId_ << ")" << std::endl;
    
    nextTickerId_++;
}

void IBKRConnection::processMessages() {
    // Message processing is handled by EReader in background
    // This is a no-op for now - messages come via callbacks
}

// ========== IBKR Callbacks ==========

void IBKRConnection::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {
    // field == 4 is LAST price (actual trade price)
    // field == 1 is BID
    // field == 2 is ASK

    if (field == 4) {  // LAST price
        // Look up which symbol this tickerId belongs to
        auto it = tickerIdToSymbol_.find(tickerId);
        if (it != tickerIdToSymbol_.end()) {
            const std::string& symbol = it->second;
            symbolManager_.updatePrice(symbol, price);
            std::cout << "💰 " << symbol << ": $" << price << std::endl;
        }
    }
}

void IBKRConnection::tickSize(TickerId tickerId, TickType field, Decimal size) {
    // field == 5 is LAST size (volume of last trade)
    // field == 0 is BID size
    // field == 3 is ASK size
}

void IBKRConnection::error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) {
    std::cout << "⚠️  Error [" << errorCode << "]: " << errorString << std::endl;
}

void IBKRConnection::nextValidId(OrderId orderId) {
    nextOrderId_ = orderId;
    std::cout << "✅ Connection established. Next Order ID: " << orderId << std::endl;
}

void IBKRConnection::connectionClosed() {
    std::cout << "🔌 Connection closed by IB Gateway" << std::endl;
}

