#include "strategy/analyzers/CamarillaCalculator.hpp"

CamarillaLevels CamarillaCalculator::calculate(const OHLCData& ohlc) {
    // Extract OHLC values
    double H = ohlc.high;
    double L = ohlc.low;
    double C = ohlc.close;
    
    // Calculate range
    double range = H - L; 

    // Calculate camarilla levels
    CamarillaLevels levels; 

    // Resistance levels
    levels.r3 = C + (range * 1.1 / 4.0);             // key resistance (reversal level)
    levels.r4 = C + (range * 1.1 / 2.0);             // Breakout resistance 
    levels.r6 = (C * H) / L;                         // Overdrive resistance (extreme high)
    levels.s3 = C - (range * 1.1 / 4.0);             // Key support (reversal level
    levels.s4 = C - (range * 1.1 / 2.0);             // Breakout support
    levels.s6 = C - (levels.r6 - C);                 // Overdrive support (extreme low)
    
    return levels; 
} 


