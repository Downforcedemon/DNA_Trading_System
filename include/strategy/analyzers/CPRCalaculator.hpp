#pragma once
#include "strategy/StrategyTypes.hpp"

class CPRCalculator { 
    public:
        // Calculate Central Pivot Range from previous day OHLC
        static CPRData calculate(const OHLCData& ohlc); 
}; 


