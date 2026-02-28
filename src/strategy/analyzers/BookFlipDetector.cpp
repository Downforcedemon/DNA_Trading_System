#include "strategy/analyzers/BookFlipDetector.hpp"
#include <cstddef>
#include <vector>
#include <cmath>

// Constructor: Initialize member variables via initializer list
// Heaps and vectors are default-constructed (empty) automatically
BookFlipDetector::BookFlipDetector(int windowSize, double percentile, int minObservations)
    : windowSize_(windowSize)
    , percentile_(percentile)
    , minObservations_(minObservations)
    , isInitialized_(false)
    , observationCount_(0) {
}

BookFlipResult BookFlipDetector::detect(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks) {

    // Step 1: First call — no previous book to compare against yet
    // Save current book as baseline and return "nothing detected"
    if (!isInitialized_) {
        previousBids_ = bids;
        previousAsks_ = asks;
        isInitialized_ = true;
        return {false, FlipType::NONE, BookSide::NONE, 0.0, 0};
    }

    // Step 2: Find the single biggest size change across both sides
    // We track the "winner" — the price level with the largest absolute delta
    int maxDelta = 0;
    double flipPrice = 0.0;
    BookSide flipSide = BookSide::NONE;
    int rawDelta = 0;  // keeps the sign: positive = size appeared, negative = size disappeared

    // Scan bid side: compare each level's current size vs previous size
    for (int i = 0; i < bids.size() && i < previousBids_.size(); i++) {
        int delta = bids[i].size - previousBids_[i].size;
        int absDelta = std::abs(delta);

        if (absDelta > maxDelta) {
            maxDelta = absDelta;
            flipPrice = bids[i].price;
            flipSide = BookSide::BID;
            rawDelta = delta;
        }
    }

    // Scan ask side: same logic, same variables (bigger change overwrites)
    for (int i = 0; i < asks.size() && i < previousAsks_.size(); i++) {
        int delta = asks[i].size - previousAsks_[i].size;
        int absDelta = std::abs(delta);

        if (absDelta > maxDelta) {
            maxDelta = absDelta;
            flipPrice = asks[i].price;
            flipSide = BookSide::ASK;
            rawDelta = delta;
        }
    }

    // Step 3: Feed maxDelta into the rolling percentile machinery

    // 3A: Add to rolling window (deque). Remove oldest if full.
    sizeWindow_.push_back(maxDelta);
    observationCount_++;

    if (sizeWindow_.size() > windowSize_) {
        sizeWindow_.pop_front();
    }

    // 3B: Insert into the correct heap
    // If smaller than the top of the bottom pile → goes in bottom pile
    // Otherwise → goes in top pile
    if (lowerHeap_.empty() || maxDelta <= lowerHeap_.top()) {
        lowerHeap_.push(maxDelta);
    } else {
        upperHeap_.push(maxDelta);
    }

    // 3C: Rebalance heaps to maintain 95/5 split
    // After this, upperHeap_.top() = 95th percentile threshold
    int lowerTarget = static_cast<int>(sizeWindow_.size() * percentile_);

    while (lowerHeap_.size() > lowerTarget) {
        upperHeap_.push(lowerHeap_.top());
        lowerHeap_.pop();
    }
    while (lowerHeap_.size() < lowerTarget && !upperHeap_.empty()) {
        lowerHeap_.push(upperHeap_.top());
        upperHeap_.pop();
    }

    // Step 4: Check if the change exceeds the 95th percentile threshold
    // Three conditions: warm-up complete, heap has data, change is statistically large
    bool isFlip = (observationCount_ >= minObservations_)
                  && !upperHeap_.empty()
                  && (maxDelta > upperHeap_.top());

    // Determine direction: BID and ASK have opposite meanings
    // BID: +delta = buyers appeared (bullish), -delta = buyers pulled (bearish)
    // ASK: +delta = sellers appeared (bearish), -delta = sellers pulled (bullish)
    FlipType flipType = FlipType::NONE;
    if (isFlip) {
        if (flipSide == BookSide::BID) {
            flipType = (rawDelta > 0) ? FlipType::BULLISH_FLIP : FlipType::BEARISH_FLIP;
        } else {
            flipType = (rawDelta > 0) ? FlipType::BEARISH_FLIP : FlipType::BULLISH_FLIP;
        }
    }

    // Step 5: Save current book as previous for next call
    previousBids_ = bids;
    previousAsks_ = asks;

    return {isFlip, flipType, flipSide, flipPrice, rawDelta};
}
