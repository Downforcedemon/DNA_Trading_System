#include "strategy/SignalAggregator.hpp"
#include <cmath>

SignalAggregator::SignalAggregator(double camarillaProximity, double cprProximity,
                                   double stackingThreshold)
    : camarillaProximity_(camarillaProximity)
    , cprProximity_(cprProximity)
    , stackingThreshold_(stackingThreshold) {
}

AggregatedSignal SignalAggregator::aggregate(
    double currentPrice,
    const CPRData& cpr,
    const CamarillaLevels& camarilla,
    const VPAResult& vpa,
    const BookFlipResult& bookFlip,
    const AbsorptionResult& absorption,
    double stackingRatio,
    const TapeReaderResult& tape) {

    // Score each factor (0 or 1)
    int cpr_s      = scoreCPR(currentPrice, cpr);
    int cam_s      = scoreCamarilla(currentPrice, camarilla);
    int vpa_s      = scoreVPA(vpa);
    int flip_s     = scoreBookFlip(bookFlip);
    int absorb_s   = scoreAbsorption(absorption);
    int stack_s    = scoreStacking(stackingRatio);

    // Composite score 0-6
    int total = cpr_s + cam_s + vpa_s + flip_s + absorb_s + stack_s;

    // Map score to strength
    SignalStrength strength;
    if (total >= 5) {
        strength = SignalStrength::HIGH;
    } else if (total >= 3) {
        strength = SignalStrength::MODERATE;
    } else {
        strength = SignalStrength::LOW;
    }

    // Determine overall direction from directional signals
    SignalDirection direction = determineDirection(bookFlip, absorption, stackingRatio, tape);

    return {total, strength, direction,
            cpr_s, cam_s, vpa_s, flip_s, absorb_s, stack_s,
            tape.cumulativeDelta, tape.largeBlockDetected, tape.divergenceDetected};
}

// +1 if price is above pivot (bullish bias) or below pivot (bearish bias)
// Score 1 if price has a clear bias relative to the pivot — not sitting on it
int SignalAggregator::scoreCPR(double currentPrice, const CPRData& cpr) {
    double distance = std::abs(currentPrice - cpr.pivot) / cpr.pivot;
    // Price must be outside the CPR range (above TC or below BC) to count as aligned
    if (currentPrice > cpr.tc || currentPrice < cpr.bc) {
        return 1;
    }
    return 0;
}

// +1 if price is near a key Camarilla level (R3, R4, S3, S4)
// These are reversal/breakout zones — being near one means a trade setup exists
int SignalAggregator::scoreCamarilla(double currentPrice, const CamarillaLevels& camarilla) {
    double levels[] = {camarilla.r3, camarilla.r4, camarilla.r6,
                       camarilla.s3, camarilla.s4, camarilla.s6};

    for (double level : levels) {
        if (level == 0.0) continue;    // skip unset levels
        double distance = std::abs(currentPrice - level) / level;
        if (distance <= camarillaProximity_) {
            return 1;
        }
    }
    return 0;
}

// +1 if VPA confirms the price move (high volume in direction of price)
int SignalAggregator::scoreVPA(const VPAResult& vpa) {
    return vpa.confirmed ? 1 : 0;
}

// +1 if a significant book flip was detected
int SignalAggregator::scoreBookFlip(const BookFlipResult& bookFlip) {
    return bookFlip.detected ? 1 : 0;
}

// +1 if absorption was detected (wall held against pressure)
int SignalAggregator::scoreAbsorption(const AbsorptionResult& absorption) {
    return absorption.detected ? 1 : 0;
}

// +1 if order book is stacked in one direction
// Ratio > threshold = bullish stacking, < 1/threshold = bearish stacking
int SignalAggregator::scoreStacking(double stackingRatio) {
    if (stackingRatio > stackingThreshold_ || stackingRatio < (1.0 / stackingThreshold_)) {
        return 1;
    }
    return 0;
}

// Determine overall signal direction from directional components
// Counts bullish vs bearish votes from signals that have a direction
SignalDirection SignalAggregator::determineDirection(
    const BookFlipResult& bookFlip,
    const AbsorptionResult& absorption,
    double stackingRatio,
    const TapeReaderResult& tape) {

    int bullishVotes = 0;
    int bearishVotes = 0;

    // BookFlip direction
    if (bookFlip.detected) {
        if (bookFlip.type == FlipType::BULLISH_FLIP) bullishVotes++;
        if (bookFlip.type == FlipType::BEARISH_FLIP) bearishVotes++;
    }

    // Absorption direction — wall on bid side held = bullish support
    if (absorption.detected) {
        if (absorption.side == BookSide::BID) bullishVotes++;    // bid wall held = support
        if (absorption.side == BookSide::ASK) bearishVotes++;    // ask wall held = resistance
    }

    // Stacking direction
    if (stackingRatio > stackingThreshold_) bullishVotes++;       // more bids = bullish
    if (stackingRatio < (1.0 / stackingThreshold_)) bearishVotes++; // more asks = bearish

    // Cumulative delta direction
    if (tape.cumulativeDelta > 0) bullishVotes++;
    if (tape.cumulativeDelta < 0) bearishVotes++;

    // Divergence downgrades conviction — reduce the leading side
    if (tape.divergenceDetected) {
        if (tape.isBearishDivergence && bullishVotes > 0) bullishVotes--;
        if (!tape.isBearishDivergence && bearishVotes > 0) bearishVotes--;
    }

    // Determine direction
    if (bullishVotes > bearishVotes) return SignalDirection::BULLISH;
    if (bearishVotes > bullishVotes) return SignalDirection::BEARISH;
    return SignalDirection::NEUTRAL;
}
