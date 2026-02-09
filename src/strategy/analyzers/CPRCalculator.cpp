#include "strategy/analyzers/CPRCalculator.hpp"

CPRData CPRCalculator::calculate(const OHLCData& ohlc) { 
    // Extract OHLC values
    double H = ohlc.high;
    double L = ohlc.low; 
    double C = ohlc.close; 

    // Calculate CPR levels
    double pivot = (H + L + C) / 3.0;
    double bc = (H + L) / 2.0; 
    double tc = (pivot - bc) + pivot; 

    // Create and populate the result
    CPRData cpr; 
    cpr.pivot = pivot; 
    cpr.bc = bc; 
    cpr.tc = tc; 

    return cpr;
} 

