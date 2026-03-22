#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "strategy/StrategyTypes.hpp"

class IMarketDataListener {
    public:
        virtual ~IMarketDataListener() = default;

        // L1 callbacks (price/size ticks)
        virtual void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) = 0;
        virtual void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) = 0;
        virtual void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) = 0;

        // L2 callbacks (order book + trades) — default empty so L1-only listeners don't need them
        virtual void onBookUpdate(const std::string& symbol,
                                  const std::vector<PriceLevel>& bids,
                                  const std::vector<PriceLevel>& asks) {}
        virtual void onTradeUpdate(const std::string& symbol, double price, int size,
                                   BookSide aggressor, double bid, double ask) {}
}; 

