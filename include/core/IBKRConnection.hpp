#pragma once
#include "DefaultEWrapper.h"
#include "EClientSocket.h"
#include "core/IMarketDataListener.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class EReader;
class EReaderOSSignal; 

class IBKRConnection : public DefaultEWrapper {
public:
    IBKRConnection(IMarketDataListener* listener);
    ~IBKRConnection();
    
    // Connection methods
    bool connect(const std::string& host, int port, int clientId);
    void disconnect();
    bool isConnected();
    
    // Subscribe to market data for a symbol
    void subscribeMarketData(const std::string& symbol);
    
    // Process incoming messages
    void processMessages();
    
    // EWrapper callbacks (IBKR API requires these)
    void tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;
    void tickSize(TickerId tickerId, TickType field, Decimal size) override;
    void error(int id, time_t errorTime, int errorCode, const std::string& errorString, const std::string& advancedOrderRejectJson) override;
    void nextValidId(OrderId orderId) override;
    void connectionClosed() override;
    
private:
    IMarketDataListener* listener_; 
    std::unique_ptr<EClientSocket> client_;
    int nextOrderId_;
    int nextTickerId_;
    std::unordered_map<int, std::string> tickerIdToSymbol_;     // Map tickerId -> symbol name
    std::unique_ptr<EReader> reader_;
    std::unique_ptr<EReaderOSSignal> signal_;
};

