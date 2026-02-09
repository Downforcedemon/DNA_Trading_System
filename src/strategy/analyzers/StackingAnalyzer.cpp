#include "strategy/analyzers/StackingAnalyzer.hpp"

double StackingAnalyzer::calculateRatio(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks){
    // Calculate total bid size (sum of all bid sizes)
    double totalBidSize = 0.0; 
    for (const PriceLevel& level : bids) {
        totalBidSize += level.size;
    } 
    // Calculate total ask size (sum of all ask sizes)
    double totalAskSize = 0.0; 
    for (const PriceLevel& level : asks) { 
        totalAskSize += level.size; 
    } 

    // Avoid division by zero
    if (totalAskSize == 0.0){
        return 0.0; 
    }

    // Calculate bid/ask ratio
    // Ratio > 1.0 = more bids ( bullish ) 
    // Ratio < 1.0 = more asks (bearish ) 
    return totalBidSize / totalAskSize; 
} 


