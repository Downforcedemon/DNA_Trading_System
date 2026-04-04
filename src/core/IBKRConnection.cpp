#include "core/IBKRConnection.hpp"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "Contract.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

static const int L2_NUM_ROWS = 10;  // request 10 levels of depth

IBKRConnection::IBKRConnection(IMarketDataListener* listener)
    : listener_(listener)
    , client_(nullptr)
    , nextOrderId_(0)
    , nextTickerId_(1)
    , nextDepthId_(10000)     // L2 IDs start at 10000 to avoid collision with L1
    , nextHistReqId_(20000) { // Historical IDs start at 20000
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
        // ask for delayed data for now
        client_->reqMarketDataType(3);
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
    int depthId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // L1 subscription
        tickerId = nextTickerId_;
        tickerIdToSymbol_[tickerId] = symbol;
        symbolToTickerId_[symbol] = tickerId;
        nextTickerId_++;

        // L2 subscription
        depthId = nextDepthId_;
        depthIdToSymbol_[depthId] = symbol;
        symbolToDepthId_[symbol] = depthId;
        orderBooks_[depthId] = LocalOrderBook{};  // empty book
        nextDepthId_++;
    }

    // L1: top-of-book price ticks
    client_->reqMktData(tickerId, contract, "", false, false, TagValueListSPtr());

    // L2: order book depth (10 levels, SMART aggregated)
    client_->reqMktDepth(depthId, contract, L2_NUM_ROWS, true, TagValueListSPtr());

    std::cout << "Subscribed to " << symbol << " L1(ID:" << tickerId << ") L2(ID:" << depthId << ")" << std::endl;
}

void IBKRConnection::unsubscribeMarketData(const std::string& symbol) {
    if (!isConnected()) return;

    int tickerId = -1;
    int depthId = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // L1 cleanup
        auto it = symbolToTickerId_.find(symbol);
        if (it != symbolToTickerId_.end()) {
            tickerId = it->second;
            symbolToTickerId_.erase(it);
            tickerIdToSymbol_.erase(tickerId);
            lastTradePrice_.erase(tickerId);
            lastTradeSize_.erase(tickerId);
        }

        // L2 cleanup
        auto dit = symbolToDepthId_.find(symbol);
        if (dit != symbolToDepthId_.end()) {
            depthId = dit->second;
            symbolToDepthId_.erase(dit);
            depthIdToSymbol_.erase(depthId);
            orderBooks_.erase(depthId);
        }

        lastBid_.erase(symbol);
        lastAsk_.erase(symbol);
    }

    if (tickerId >= 0) client_->cancelMktData(tickerId);
    if (depthId >= 0)  client_->cancelMktDepth(depthId, true);

    std::cout << "Unsubscribed from " << symbol << std::endl;
}

void IBKRConnection::processMessages() {
    if (signal_ && reader_) {
        signal_->waitForSignal();
        reader_->processMsgs();
    }
}

// ========== L1 Callbacks ==========

void IBKRConnection::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tickerIdToSymbol_.find(tickerId);
    if (it == tickerIdToSymbol_.end()) return;
    const std::string& symbol = it->second;

    if (field == 1) {  // BID price
        lastBid_[symbol] = price;
    } else if (field == 2) {  // ASK price
        lastAsk_[symbol] = price;
    } else if (field == 4) {  // LAST price (trade occurred)
        lastTradePrice_[tickerId] = price;
        listener_->onPriceUpdate(symbol, price, std::time(nullptr));

        // If we already have the size, fire the trade
        auto sizeIt = lastTradeSize_.find(tickerId);
        if (sizeIt != lastTradeSize_.end() && sizeIt->second > 0) {
            double bid = lastBid_.count(symbol) ? lastBid_[symbol] : 0.0;
            double ask = lastAsk_.count(symbol) ? lastAsk_[symbol] : 0.0;

            // Classify aggressor: trade at/above ask = buyer, at/below bid = seller
            BookSide aggressor = BookSide::NONE;
            if (ask > 0.0 && price >= ask) aggressor = BookSide::ASK;
            else if (bid > 0.0 && price <= bid) aggressor = BookSide::BID;

            listener_->onTradeUpdate(symbol, price, sizeIt->second, aggressor, bid, ask);
            lastTradeSize_.erase(sizeIt);
        }
    }
}

void IBKRConnection::tickSize(TickerId tickerId, TickType field, Decimal size) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tickerIdToSymbol_.find(tickerId);
    if (it == tickerIdToSymbol_.end()) return;
    const std::string& symbol = it->second;
    int intSize = static_cast<int>(size);

    if (field == 5) {  // LAST size
        lastTradeSize_[tickerId] = intSize;
        listener_->onSizeUpdate(symbol, intSize, std::time(nullptr));

        // If we already have the price, fire the trade
        auto priceIt = lastTradePrice_.find(tickerId);
        if (priceIt != lastTradePrice_.end() && priceIt->second > 0.0) {
            double bid = lastBid_.count(symbol) ? lastBid_[symbol] : 0.0;
            double ask = lastAsk_.count(symbol) ? lastAsk_[symbol] : 0.0;

            BookSide aggressor = BookSide::NONE;
            if (ask > 0.0 && priceIt->second >= ask) aggressor = BookSide::ASK;
            else if (bid > 0.0 && priceIt->second <= bid) aggressor = BookSide::BID;

            listener_->onTradeUpdate(symbol, priceIt->second, intSize, aggressor, bid, ask);
            lastTradePrice_.erase(priceIt);
        }
    }
}

// ========== L2 Callbacks (Order Book Depth) ==========

void IBKRConnection::updateMktDepth(TickerId id, int position, int operation, int side,
                                     double price, Decimal size) {
    applyDepthUpdate(id, position, operation, side, price, static_cast<int>(size));
}

void IBKRConnection::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker,
                                       int operation, int side, double price, Decimal size,
                                       bool isSmartDepth) {
    // SMART depth aggregates across exchanges — same handling as L1 depth
    applyDepthUpdate(id, position, operation, side, price, static_cast<int>(size));
}

void IBKRConnection::applyDepthUpdate(int tickerId, int position, int operation, int side,
                                       double price, int size) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto bookIt = orderBooks_.find(tickerId);
    if (bookIt == orderBooks_.end()) return;

    auto symIt = depthIdToSymbol_.find(tickerId);
    if (symIt == depthIdToSymbol_.end()) return;

    // side: 0 = ask, 1 = bid
    auto& levels = (side == 1) ? bookIt->second.bids : bookIt->second.asks;

    // operation: 0 = insert, 1 = update, 2 = delete
    switch (operation) {
        case 0: {  // INSERT at position
            PriceLevel level{price, size};
            if (position >= static_cast<int>(levels.size())) {
                levels.push_back(level);
            } else {
                levels.insert(levels.begin() + position, level);
            }
            break;
        }
        case 1: {  // UPDATE at position
            if (position < static_cast<int>(levels.size())) {
                levels[position].price = price;
                levels[position].size = size;
            }
            break;
        }
        case 2: {  // DELETE at position
            if (position < static_cast<int>(levels.size())) {
                levels.erase(levels.begin() + position);
            }
            break;
        }
    }

    // Forward full book snapshot to listener
    const std::string& symbol = symIt->second;
    listener_->onBookUpdate(symbol, bookIt->second.bids, bookIt->second.asks);
}

// ========== Historical Data (OHLC) ==========

void IBKRConnection::requestHistoricalData(const std::string& symbol) {
    if (!isConnected()) return;

    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";

    int reqId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        reqId = nextHistReqId_++;
        histReqIdToSymbol_[reqId] = symbol;
        pendingOHLC_[reqId] = OHLCData{};
    }

    // 2 days of daily bars, RTH only — last bar received is the most recent complete day
    client_->reqHistoricalData(reqId, contract, "", "2 D", "1 day", "TRADES",
                               1, 1, false, TagValueListSPtr());
    std::cout << "Requested OHLC for " << symbol << " (reqId:" << reqId << ")" << std::endl;
}

void IBKRConnection::historicalData(TickerId reqId, const Bar& bar) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = histReqIdToSymbol_.find(reqId);
    if (it == histReqIdToSymbol_.end()) return;

    // Overwrite each time — after historicalDataEnd fires, pendingOHLC_ holds the last (most recent) bar
    OHLCData& ohlc = pendingOHLC_[reqId];
    ohlc.open  = bar.open;
    ohlc.high  = bar.high;
    ohlc.low   = bar.low;
    ohlc.close = bar.close;
    ohlc.timestamp = std::time(nullptr);
}

void IBKRConnection::historicalDataEnd(int reqId, const std::string& startDateStr,
                                        const std::string& endDateStr) {
    std::string symbol;
    OHLCData ohlc;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = histReqIdToSymbol_.find(reqId);
        if (it == histReqIdToSymbol_.end()) return;
        symbol = it->second;
        ohlc = pendingOHLC_[reqId];
        histReqIdToSymbol_.erase(reqId);
        pendingOHLC_.erase(reqId);
    }

    if (ohlc.close > 0.0) {
        std::cout << "OHLC " << symbol << ": O=" << ohlc.open << " H=" << ohlc.high
                  << " L=" << ohlc.low << " C=" << ohlc.close << std::endl;
        listener_->onOHLCUpdate(symbol, ohlc);
    }
}

// ========== Connection Callbacks ==========

void IBKRConnection::error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) {
    std::cout << "Error [" << errorCode << "]: " << errorString << std::endl;

    std::lock_guard<std::mutex> lock(mutex_);
    // Check both L1 and L2 ID spaces
    auto it = tickerIdToSymbol_.find(id);
    if (it != tickerIdToSymbol_.end()) {
        listener_->onError(it->second, errorCode, errorString);
        return;
    }
    auto dit = depthIdToSymbol_.find(id);
    if (dit != depthIdToSymbol_.end()) {
        listener_->onError(dit->second, errorCode, errorString);
    }
}

void IBKRConnection::nextValidId(OrderId orderId) {
    nextOrderId_ = orderId;
    std::cout << "Connection ready. Next Order ID: " << orderId << std::endl;
}

void IBKRConnection::connectionClosed() {
    std::cout << "Connection closed by IB Gateway" << std::endl;
}
