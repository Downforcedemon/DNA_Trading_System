#pragma once
#include "strategy/StrategyTypes.hpp"

class CamarillaCalculator {
    public:
        // Calculate Camarilla pivot levels from previous day OHLC
        static CamarillaLevels calculate(const OHLCData& ohlc);
}; 
