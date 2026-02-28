#include "strategy/analyzers/VPAAnalyser.hpp"


VPAAnalyzer::VPAAnalyzer(int windowSize, double highVolumeMultiplier,
                         double lowVolumeMultiplier, int minObservations)
    : windowSize_(windowSize)
    , highVolumeMultiplier_(highVolumeMultiplier)
    , lowVolumeMultiplier_(lowVolumeMultiplier)
    , previousPrice_(0.0)
    , volumeSum_(0)
    , isInitialized_(false)
    , observationCount_(0){
    }

VPAResult VPAAnalyzer::detect(double currentPrice, int currentVolume){
    // Step 1: first call
    if (!isInitialized_){
        previousPrice_ = currentPrice;
        isInitialized_ = true;
        return {false, false, 0.0, 0};
    }

    // Step 2: Update rolling volume window (sliding window sum)
    volumeHistory_.push_back(currentVolume);
    volumeSum_ += currentVolume;
    observationCount_++;

    if (volumeHistory_.size() > windowSize_) {
        volumeSum_ -= volumeHistory_.front();
        volumeHistory_.pop_front();
    }

    // Step 3: Calculate average volume — O(1) from running sum
    double avgVolume = static_cast<double>(volumeSum_) / volumeHistory_.size();

    // Step 4: Calculate price change and volume ratio
    double priceChange = currentPrice - previousPrice_;
    int volumeRatio = (avgVolume > 0) ? static_cast<int>(currentVolume / avgVolume) : 0;

    // Step 5: Warm-up check — need enough data for a meaningful average
    if (observationCount_ < minObservations_) {
        previousPrice_ = currentPrice;
        return {false, false, priceChange, volumeRatio};
    }

    // Step 6: Determine confirmation vs divergence
    bool isHighVolume = currentVolume > (avgVolume * highVolumeMultiplier_);
    bool isLowVolume = currentVolume < (avgVolume * lowVolumeMultiplier_);
    bool priceUp = priceChange > 0;
    bool priceDown = priceChange < 0;

    // Confirmation: volume agrees with price direction
    // Price UP + High volume = real move (confirmed)
    // Price DOWN + High volume = real selloff (confirmed)
    bool confirmed = (priceUp || priceDown) && isHighVolume;

    // Divergence: volume contradicts price direction
    // Price UP + Low volume = fake move (bearish divergence)
    // Price DOWN + Low volume = fake selloff (bullish divergence)
    bool isDivergence = (priceUp || priceDown) && isLowVolume;

    // Step 7: Save current price for next call, return result
    previousPrice_ = currentPrice;
    return {confirmed, isDivergence, priceChange, volumeRatio};
}
