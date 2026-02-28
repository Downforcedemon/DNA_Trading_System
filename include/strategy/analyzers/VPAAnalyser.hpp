#pragma once
#include "strategy/StrategyTypes.hpp"
#include <deque>
#include <mutex>

class VPAAnalyzer {
    public:
        VPAAnalyzer(int windowSize = 100, double highVolumeMultiplier = 1.5,
                    double lowVolumeMultiplier = 0.5, int minObservations = 20);
        VPAResult detect(double currentPrice, int currentVolume);

    private:
        // Previous price - to calculate direction
        double previousPrice_;

        // Volume history - rolling window for average calculation
        std::deque<int> volumeHistory_;
        int volumeSum_;                    // running sum to avoid recalculating every time
        
        // config
        int windowSize_;
        double highVolumeMultiplier_;     // 1.5 = volume must be 1.5x average to be "high"
        double lowVolumeMultiplier_;      // 0.5 = volume beone 0.5x average is "low"
        int minObservations_;

        // Warm-up
        bool isInitialized_;
        int observationCount_;
        mutable std::mutex mutex_;
}; 
