#pragma once
#include "strategy/StrategyTypes.hpp"
#include <vector>

class StackingAnalyzer { 
    public:
        // Calculate bid/ask imbalance ratio from order book
        // Returns ratio: bidSize / askSize (>1 = bullish, <1 = bearish)
        static double calculateRatio(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks);
            
