#pragma once
#include "strategy/StrategyTypes.hpp"
#include <vector>

class SignalAggregator {
    public:
        // Configure proximity thresholds for level-based signals
        SignalAggregator(double camarillaProximity = 0.002,   // 0.2% from level
                         double cprProximity = 0.002,          // 0.2% from pivot
                         double stackingThreshold = 1.5);      // bid/ask ratio > 1.5 = bullish

        // Aggregate all analyzer outputs into a single composite signal
        AggregatedSignal aggregate(
            double currentPrice,
            const CPRData& cpr,
            const CamarillaLevels& camarilla,
            const VPAResult& vpa,
            const BookFlipResult& bookFlip,
            const AbsorptionResult& absorption,
            double stackingRatio,
            const TapeReaderResult& tape
        );

    private:
        double camarillaProximity_;    // how close to a level counts as "at level"
        double cprProximity_;          // how close to pivot counts as "aligned"
        double stackingThreshold_;     // ratio above this = bullish stacking

        // Individual scoring methods
        int scoreCPR(double currentPrice, const CPRData& cpr);
        int scoreCamarilla(double currentPrice, const CamarillaLevels& camarilla);
        int scoreVPA(const VPAResult& vpa);
        int scoreBookFlip(const BookFlipResult& bookFlip);
        int scoreAbsorption(const AbsorptionResult& absorption);
        int scoreStacking(double stackingRatio);

        // Determine overall direction from individual signals
        SignalDirection determineDirection(const BookFlipResult& bookFlip,
                                           const AbsorptionResult& absorption,
                                           double stackingRatio,
                                           const TapeReaderResult& tape);
};
