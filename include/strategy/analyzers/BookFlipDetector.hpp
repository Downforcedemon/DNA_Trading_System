#pragma once

#include "strategy/StrategyTypes.hpp"
#include <vector>
#include <ctime>

enum class BookFlipSignal {
    BULLISH,            // Large bid appears or large ask disappears
    BEARISH,            // Large ask appears or large bid disappears
    NONE                // No flip detected
}; 

class BookFlipDetector {
    public:
        // Constructor
        BookFlipDetector(); 

        // Update with new order book and detect flips
        // Returns BULLISH, BEARISH or NONE
        BookFlipSignal update(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks); 

    private:
        // Previous order book state
        std::vector<PriceLevel> previousBids_;
        std::vector<PriceLevel> previousAsks_;

        // Cooldown tracking (5-second cooldown)
        time_t lastFlipTime_; 

        // Helper: check if a level is "large" (3x average of top 5)
        bool isLargeOrder(int size, const std::vector<PriceLevel>& levels) const; 

        // Helper: Find order at specific price in book
        bool findOrder(double price, const std::vector<PriceLevel>& levels, int& outSize) const; 
}; 


