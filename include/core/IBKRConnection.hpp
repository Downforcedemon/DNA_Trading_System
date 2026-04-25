#pragma once
#include "DefaultEWrapper.h"
#include "EClientSocket.h"
#include "bar.h"
#include "core/IMarketDataListener.hpp"
#include "strategy/StrategyTypes.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

class EReader;
class EReaderOSSignal;

// Local order book maintained from incremental IBKR depth updates
struct LocalOrderBook {
    std::vector<PriceLevel> bids;   // sorted best (highest) first
    std::vector<PriceLevel> asks;   // sorted best (lowest) first
};

class IBKRConnection : public DefaultEWrapper {
public:
    IBKRConnection(IMarketDataListener* listener);
    ~IBKRConnection();

    // Connection methods
    bool connect(const std::string& host, int port, int clientId);
    void disconnect();
    bool isConnected();

    // Subscribe/unsubscribe market data (L1 + L2)
    // exchange: listing venue for L2 routing (NASDAQ | NYSE | ARCA | BATS | AMEX).
    // Pass empty string to auto-discover via reqContractDetails.
    void subscribeMarketData(const std::string& symbol, const std::string& exchange = "");
    void unsubscribeMarketData(const std::string& symbol);

    // Process incoming messages
    void processMessages();

    // EWrapper callbacks — L1
    void tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;
    void tickSize(TickerId tickerId, TickType field, Decimal size) override;
    void error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) override;
    void nextValidId(OrderId orderId) override;
    void connectionClosed() override;

    // EWrapper callbacks — L2 (order book depth)
    void updateMktDepth(TickerId id, int position, int operation, int side,
                        double price, Decimal size) override;
    void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker,
                          int operation, int side, double price, Decimal size,
                          bool isSmartDepth) override;

    // Request previous day's OHLC via reqHistoricalData
    void requestHistoricalData(const std::string& symbol);

    // EWrapper callbacks — historical data
    void historicalData(TickerId reqId, const Bar& bar) override;
    void historicalDataEnd(int reqId, const std::string& startDateStr,
                           const std::string& endDateStr) override;

    // EWrapper callbacks — contract details (used to auto-discover listing exchange)
    void contractDetails(int reqId, const ContractDetails& contractDetails) override;
    void contractDetailsEnd(int reqId) override;

private:
    IMarketDataListener* listener_;
    std::unique_ptr<EClientSocket> client_;
    int nextOrderId_;
    int nextTickerId_;
    int nextDepthId_;                                            // separate ID space for L2
    int nextHistReqId_;                                          // separate ID space for historical data
    int nextContractDetailsReqId_;                               // separate ID space for contract-detail lookups

    // Pending exchange-discovery lookups: reqId -> symbol awaiting subscribe
    std::unordered_map<int, std::string> pendingExchangeLookups_;

    // L1 ticker ID mappings
    std::unordered_map<int, std::string> tickerIdToSymbol_;
    std::unordered_map<std::string, int> symbolToTickerId_;

    // L2 depth ID mappings
    std::unordered_map<int, std::string> depthIdToSymbol_;
    std::unordered_map<std::string, int> symbolToDepthId_;

    // Local order books built from incremental updates
    std::unordered_map<int, LocalOrderBook> orderBooks_;

    // L1 bid/ask tracking for trade aggressor detection
    std::unordered_map<std::string, double> lastBid_;
    std::unordered_map<std::string, double> lastAsk_;
    // Last trade price/size for pairing tickPrice + tickSize
    std::unordered_map<int, double> lastTradePrice_;
    std::unordered_map<int, int> lastTradeSize_;

    mutable std::mutex mutex_;
    std::unique_ptr<EReader> reader_;
    std::unique_ptr<EReaderOSSignal> signal_;

    // Historical data request tracking
    std::unordered_map<int, std::string> histReqIdToSymbol_;
    std::unordered_map<int, OHLCData>   pendingOHLC_;           // accumulates bars; last one wins

    // Apply incremental depth update to local book and forward snapshot
    void applyDepthUpdate(int tickerId, int position, int operation, int side,
                          double price, int size);

    // Issue the actual L1 + L2 requests once the listing exchange is known
    void doSubscribeImpl(const std::string& symbol, const std::string& exchange);
};

