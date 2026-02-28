#pragma once 
#include "strategy/StrategyTypes.hpp"
#include <vector>
#include <deque>
#include <queue>
#include <mutex>
#include <functional>

class BookFlipDetector {
    public:
        BookFlipDetector(int windowSize = 1000, double percentile = 0.95, int minObservations = 30);
        BookFlipResult detect(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks);

    private:
        // What the book lookek like last tick
        std::vector<PriceLevel> previousBids_;
        std::vector<PriceLevel> previousAsks_;

        // Rolling Window + heaps - the percentile machinary
        std::deque<int> sizeWindow_;
        std::priority_queue<int> lowerHeap_;
        std::priority_queue<int, std::vector<int>, std::greater<int>> upperHeap_;

        // config
        int windowSize_;
        double percentile_;
        int minObservations_;

        // Warm-up + thead_safety
        bool isInitialized_;
        int observationCount_;
        mutable std::mutex mutex_; 

};

