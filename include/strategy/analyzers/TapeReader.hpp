#pragma once
#include "strategy/StrategyTypes.hpp"
#include <deque>
#include <mutex>
#include <cmath>

class TapeReader {
    public:
        // Constructor: configure rolling window, block threshold, warm-up period
        TapeReader(int windowSize = 500, double blockMultiplier = 2.0,
                   int minObservations = 50, time_t sessionStart = 0);

        // Process a single trade from Time & Sales
        TapeReaderResult processTrade(double price, int size, BookSide aggressor,
                                      double bid, double ask);

        // Reset cumulative delta (call at RTH open 9:30 ET)
        void resetDelta();

    private:
        // --- Cumulative Delta ---
        int cumulativeDelta_;                   // net buying - selling pressure, resets daily

        // --- Block Detection (rolling stats) ---
        std::deque<int> tradeHistory_;          // rolling window of trade sizes
        int tradeSizeSum_;                      // running sum for O(1) mean
        double tradeSizeSqSum_;                 // running sum of squares for O(1) std dev

        // --- Swing Divergence Tracking ---
        double priceHigh_;                      // current session high price
        double priceLow_;                       // current session low price
        int deltaAtPriceHigh_;                  // cum delta when price made that high
        int deltaAtPriceLow_;                   // cum delta when price made that low

        // --- Configuration ---
        int windowSize_;                        // rolling window size for block detection
        double blockMultiplier_;                // SD multiplier (default 2.0 = top 2.5%)
        int minObservations_;                   // warm-up period before signaling
        time_t sessionStart_;                   // RTH open time for delta reset

        // --- State ---
        int observationCount_;                  // total trades processed
        bool isInitialized_;                    // first trade flag
        mutable std::mutex mutex_;              // thread safety
};
