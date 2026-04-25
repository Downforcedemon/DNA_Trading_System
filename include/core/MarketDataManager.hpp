#pragma once
#include "core/IMarketDataProvider.hpp"
#include "core/IMarketDataListener.hpp"
#include "strategy/StrategyTypes.hpp"
#include <memory>
#include <vector>
#include <string>

class MarketDataManager : public IMarketDataListener { 
    public:
        MarketDataManager();
        ~MarketDataManager(); 

        // provider management
        void setProvider(std::unique_ptr<IMarketDataProvider> provider);
        IMarketDataProvider* getProvider() const;

        // Listener management
        void addListener(IMarketDataListener* listener); 
        void removeListener(IMarketDataListener* listener);

        // IMarketDataListener implementation (receive from provider, broadcast to listeners)
        void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) override;
        void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) override;
        void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) override;
        void onBookUpdate(const std::string& symbol,
                          const std::vector<PriceLevel>& bids,
                          const std::vector<PriceLevel>& asks) override;
        void onTradeUpdate(const std::string& symbol, double price, int size,
                           BookSide aggressor, double bid, double ask) override;
        void onOHLCUpdate(const std::string& symbol, const OHLCData& ohlc) override;
        void onExchangeDiscovered(const std::string& symbol, const std::string& exchange) override;

        // Request previous day's OHLC for all listeners (delegates to provider)
        void requestOHLC(const std::string& symbol);
        
    private:
        std::unique_ptr<IMarketDataProvider> provider_;
        std::vector<IMarketDataListener*> listeners_; 
}; 
