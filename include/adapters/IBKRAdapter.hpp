#pragma once
#include "core/IMarketDataProvider.hpp"
#include "core/IMarketDataListener.hpp"
#include "core/IBKRConnection.hpp"
#include "strategy/StrategyTypes.hpp"
#include <memory>
#include <string>
#include <vector>

struct IBKRConfig {
    std::string host;
    int port;
    int clientId;
};

class IBKRAdapter : public IMarketDataProvider, public IMarketDataListener {
    public:
        IBKRAdapter(IMarketDataListener* listener, const IBKRConfig& config);
        ~IBKRAdapter() override; 

        // IMarketDataProvider implementation
        bool connect() override;
        void disconnect() override;
        bool isConnected() const override; 
        void subscribe(const std::string& symbol) override; 
        void unsubscribe(const std::string& symbol) override; 
        std::string getProviderName() const override;
        void processMessages() override;

        // IMarketDataListener implementation (receive from IBKRConnection, forward to listener_)
        void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) override;
        void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) override;
        void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) override;
        void onBookUpdate(const std::string& symbol,
                          const std::vector<PriceLevel>& bids,
                          const std::vector<PriceLevel>& asks) override;
        void onTradeUpdate(const std::string& symbol, double price, int size,
                           BookSide aggressor, double bid, double ask) override;

    private:
        std::unique_ptr<IBKRConnection> ibkrConnection_;
        IMarketDataListener* listener_;
        IBKRConfig config_; 
}; 
