#include "strategy/analyzers/AbsorptionDetector.hpp"
#include <cmath>

// Constructor: Initialize config and warm-up flags
// Heaps, vectors, and hashmaps are default-constructed (empty) automatically
AbsorptionDetector::AbsorptionDetector(int windowSize, double percentile, int minObservations, double stabilityThreshold)
    : windowSize_(windowSize)
    , percentile_(percentile)
    , minObservations_(minObservations)
    , stabilityThreshold_(stabilityThreshold)
    , isInitialized_(false)
    , observationCount_(0) {
}

AbsorptionResult AbsorptionDetector::detect(
    const std::vector<PriceLevel>& bids,
    const std::vector<PriceLevel>& asks,
    const std::vector<TradeData>& recentTrades) {

    // Step 1: First call — no previous book yet, save baseline
    if (!isInitialized_) {
        previousBids_ = bids;
        previousAsks_ = asks;

        // Record initial wall sizes for all visible levels
        for (const auto& level : bids) {
            originalWallSize_[level.price] = level.size;
        }
        for (const auto& level : asks) {
            originalWallSize_[level.price] = level.size;
        }

        isInitialized_ = true;
        return {false, BookSide::NONE, 0.0, 0, 0};
    }

    // Step 2: Accumulate trade volume at each price level
    // Every trade that hits a price adds to the "attack" counter for that level
    for (const auto& trade : recentTrades) {
        volumeAbsorbed_[trade.price] += trade.size;
    }

    // Step 3: Check each price level in the current book for absorption
    // Absorption = wall is still standing AND significant volume has hit it
    int bestAbsorbed = 0;
    double bestPrice = 0.0;
    int bestWallSize = 0;
    BookSide bestSide = BookSide::NONE;

    // Check bid levels (walls being defended by buyers)
    for (const auto& level : bids) {
        double price = level.price;
        int currentSize = level.size;

        // Skip if we haven't seen this level before
        if (originalWallSize_.find(price) == originalWallSize_.end()) {
            // New level appeared — record it for future tracking
            originalWallSize_[price] = currentSize;
            continue;
        }

        int originalSize = originalWallSize_[price];

        // Skip small walls — not worth tracking
        if (originalSize < 100) continue;

        // Stability check: wall must still be above threshold (e.g., 50% of original)
        double stabilityRatio = static_cast<double>(currentSize) / originalSize;
        if (stabilityRatio < stabilityThreshold_) {
            // Wall broke — clear tracking for this level
            volumeAbsorbed_.erase(price);
            originalWallSize_.erase(price);
            continue;
        }

        // How much volume has been absorbed at this level?
        int absorbed = volumeAbsorbed_[price];
        if (absorbed > bestAbsorbed) {
            bestAbsorbed = absorbed;
            bestPrice = price;
            bestWallSize = currentSize;
            bestSide = BookSide::BID;
        }
    }

    // Check ask levels (walls being defended by sellers)
    for (const auto& level : asks) {
        double price = level.price;
        int currentSize = level.size;

        if (originalWallSize_.find(price) == originalWallSize_.end()) {
            originalWallSize_[price] = currentSize;
            continue;
        }

        int originalSize = originalWallSize_[price];

        if (originalSize < 100) continue;

        double stabilityRatio = static_cast<double>(currentSize) / originalSize;
        if (stabilityRatio < stabilityThreshold_) {
            volumeAbsorbed_.erase(price);
            originalWallSize_.erase(price);
            continue;
        }

        int absorbed = volumeAbsorbed_[price];
        if (absorbed > bestAbsorbed) {
            bestAbsorbed = absorbed;
            bestPrice = price;
            bestWallSize = currentSize;
            bestSide = BookSide::ASK;
        }
    }

    // Step 4: Feed best absorbed volume into rolling percentile machinery
    volumeWindow_.push_back(bestAbsorbed);
    observationCount_++;

    if (volumeWindow_.size() > windowSize_) {
        volumeWindow_.pop_front();
    }

    // Insert into correct heap
    if (lowerHeap_.empty() || bestAbsorbed <= lowerHeap_.top()) {
        lowerHeap_.push(bestAbsorbed);
    } else {
        upperHeap_.push(bestAbsorbed);
    }

    // Rebalance heaps to maintain 95/5 split
    int lowerTarget = static_cast<int>(volumeWindow_.size() * percentile_);

    while (lowerHeap_.size() > lowerTarget) {
        upperHeap_.push(lowerHeap_.top());
        lowerHeap_.pop();
    }
    while (lowerHeap_.size() < lowerTarget && !upperHeap_.empty()) {
        lowerHeap_.push(upperHeap_.top());
        upperHeap_.pop();
    }

    // Step 5: Check if absorption exceeds 95th percentile threshold
    bool isAbsorption = (observationCount_ >= minObservations_)
                        && !upperHeap_.empty()
                        && (bestAbsorbed > upperHeap_.top());

    // Step 6: Save current book as previous for next call
    previousBids_ = bids;
    previousAsks_ = asks;

    // Update original wall sizes for levels that are still standing
    // If the wall grew (replenished), update the baseline
    for (const auto& level : bids) {
        if (originalWallSize_.find(level.price) != originalWallSize_.end()) {
            if (level.size > originalWallSize_[level.price]) {
                originalWallSize_[level.price] = level.size;
            }
        }
    }
    for (const auto& level : asks) {
        if (originalWallSize_.find(level.price) != originalWallSize_.end()) {
            if (level.size > originalWallSize_[level.price]) {
                originalWallSize_[level.price] = level.size;
            }
        }
    }

    return {isAbsorption, bestSide, bestPrice, bestWallSize, bestAbsorbed};
}
