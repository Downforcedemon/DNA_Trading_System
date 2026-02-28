#pragma once
#include "strategy/StrategyTypes.hpp"
#include <vector>
#include <deque>
#include <queue>
#include <mutex>
#include <functional>
#include <unordered_map>

class AbsorptionDetector {
    public:
        AbsorptionDetector(int windowSize = 1000, double percentile = 0.95, int minObservations = 30, double stabilityThreshold = 0.5);
        AbsorptionResult detect(const std::vector<PriceLevel>& bids,
                                const std::vector<PriceLevel>& asks,
                                const std::vector<TradeData>& recentTrades);
    private:
        // Group 1: Previous book snapshot
        std::vector<PriceLevel> previousBids_;
        std::vector<PriceLevel> previousAsks_;

        // Group 2: Per-Level absorption tracking
        std::unordered_map<double, int> originalWallSize_; // price -> first observed wall size
        std::unordered_map<double, int> volumeAbsorbed_;    // price -> total volume hit while wall held
                                                           
        // Group 3: Rolling percentail machinery
        std::deque<int> volumeWindow_;
        std::priority_queue<int> lowerHeap_;
        std::priority_queue<int, std::vector<int>, std::greater<int>> upperHeap_;

        // Group 4: Config
        int windowSize_;
        double percentile_;
        int minObservations_;
        double stabilityThreshold_;

        // Group 5: Warm-up + thread safety
        bool isInitialized_;
        int observationCount_;
        mutable std::mutex mutex_;
}; 


