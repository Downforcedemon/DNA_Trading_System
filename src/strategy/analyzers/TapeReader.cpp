#include "strategy/analyzers/TapeReader.hpp"

TapeReader::TapeReader(int windowSize, double blockMultiplier, int minObservations,
        time_t sessionStart)
    : cumulativeDelta_(0)
    , tradeSizeSum_(0)
    , tradeSizeSqSum_(0.0)
    , priceHigh_(0.0)
    , priceLow_(0.0)
    , deltaAtPriceHigh_(0)
    , deltaAtPriceLow_(0)
    , windowSize_(windowSize)
    , blockMultiplier_(blockMultiplier)
    , minObservations_(minObservations)
    , sessionStart_(sessionStart)
    , observationCount_(0)
    , isInitialized_(false) {
    }

void TapeReader::resetDelta() {
    cumulativeDelta_ = 0;
    priceHigh_ = 0.0;
    priceLow_ = 0.0;
    deltaAtPriceHigh_ = 0;
    deltaAtPriceLow_ = 0;
}

TapeReaderResult TapeReader::processTrade(double price, int size, BookSide aggressor,
                                           double bid, double ask) {

    // Step 3: First call — set baseline price high/low, no data to analyze yet
    if (!isInitialized_) {
        priceHigh_ = price;
        priceLow_ = price;
        deltaAtPriceHigh_ = 0;
        deltaAtPriceLow_ = 0;
        isInitialized_ = true;
        return {0, false, 0, BookSide::NONE, false, false};
    }

    // Step 4: Update cumulative delta based on aggressor side
    // Buyer aggressor (hit the ask) = positive delta (buying pressure)
    // Seller aggressor (hit the bid) = negative delta (selling pressure)
    if (aggressor == BookSide::BID) {
        cumulativeDelta_ -= size;    // seller hit the bid = selling pressure
    } else if (aggressor == BookSide::ASK) {
        cumulativeDelta_ += size;    // buyer hit the ask = buying pressure
    }
    // BookSide::NONE = midpoint trade, doesn't affect delta

    // Step 5: Update rolling trade size stats (sliding window sum + sum of squares)
    tradeHistory_.push_back(size);
    tradeSizeSum_ += size;
    tradeSizeSqSum_ += static_cast<double>(size) * size;
    observationCount_++;

    if (tradeHistory_.size() > windowSize_) {
        int old = tradeHistory_.front();
        tradeSizeSum_ -= old;
        tradeSizeSqSum_ -= static_cast<double>(old) * old;
        tradeHistory_.pop_front();
    }

    // Step 6: Block detection — mean + 2×SD threshold (BookMap approach)
    bool largeBlock = false;
    BookSide blockSide = BookSide::NONE;

    if (observationCount_ >= minObservations_) {
        double count = static_cast<double>(tradeHistory_.size());
        double mean = tradeSizeSum_ / count;
        double variance = (tradeSizeSqSum_ / count) - (mean * mean);
        double stdDev = (variance > 0) ? std::sqrt(variance) : 0.0;
        double threshold = mean + (blockMultiplier_ * stdDev);

        largeBlock = (size > threshold);
        if (largeBlock) {
            blockSide = aggressor;
        }
    }

    // Step 7: Swing divergence detection
    bool divergenceDetected = false;
    bool isBearishDivergence = false;

    // Update swing high — price made a new high
    if (price > priceHigh_) {
        // Check BEFORE updating: is delta at this new high LOWER than delta at previous high?
        // That means buyers are weakening even though price is pushing higher
        if (cumulativeDelta_ < deltaAtPriceHigh_ && observationCount_ >= minObservations_) {
            divergenceDetected = true;
            isBearishDivergence = true;   // price up, delta down = bearish warning
        }
        priceHigh_ = price;
        deltaAtPriceHigh_ = cumulativeDelta_;
    }

    // Update swing low — price made a new low
    if (price < priceLow_) {
        // Is delta at this new low HIGHER than delta at previous low?
        // That means sellers are weakening even though price is dropping
        if (cumulativeDelta_ > deltaAtPriceLow_ && observationCount_ >= minObservations_) {
            divergenceDetected = true;
            isBearishDivergence = false;  // price down, delta up = bullish warning
        }
        priceLow_ = price;
        deltaAtPriceLow_ = cumulativeDelta_;
    }

    return {cumulativeDelta_, largeBlock, largeBlock ? size : 0, blockSide,
            divergenceDetected, isBearishDivergence};
}
