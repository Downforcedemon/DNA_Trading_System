#include "strategy/analyzers/BookFlipDetector.hpp"

// Constructor: Initialize with empty state and no cooldown
BookFlipDetector::BookFlipDetector() : lastFlipTime_(0); {
    // previousBids) and previousAsks_ are already emtpy (default constructed)
    }

