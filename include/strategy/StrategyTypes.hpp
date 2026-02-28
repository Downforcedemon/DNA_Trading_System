#pragma once
#include <string>
#include <ctime>

// Represents BookFlip types
enum class FlipType { BULLISH_FLIP, BEARISH_FLIP, NONE };
enum class BookSide { BID, ASK, NONE };

// Represents a single price level in order book
struct PriceLevel {
    double price; 
    int size; 
};

// OHLC Data for pivot calculations
struct OHLCData {
    double open;
    double high;
    double low;
    double close;
    time_t timestamp;
};

// CPR (Central Pivot Range) calculation results
struct CPRData { 
    double pivot;             // Main Pivot
    double bc;                // Bottom Central
    double tc;                // Top Central
}; 

// Camarilla pivot levels
struct CamarillaLevels {
    double r6;                  // Overdrive resistance (extreme high)
    double r4;                  // Breakout resistance
    double r3;                  // key resistance (reversal level)
    double s3;                  // Key support (reversal level) 
    double s4;                  // Breakout Support
    double s6;                  // Overdrive support (extreme low) 
}; 

// BookFlip
struct BookFlipResult {
    bool detected;
    FlipType type;
    BookSide side;
    double price;
    int sizeDelta;
};

// struct AbsorptionDetector needs these
struct AbsorptionResult { 
    bool detected;
    BookSide side;
    double price;
    int wallSize;
    int volumeAbsorbed;
}; 

// Absorption Trade Level input
struct TradeData {
    double price;
    int size;
    time_t timestamp;
}; 

// VPA
struct VPAResult {
    bool confirmed;                  // does volume confirm the price move ?
    bool isDivergence;               // volume contradicts price (warning signal)
    double priceChange;              // how much price mvoed
    int volumeRatio;                 // current volume vs average volume
}; 

// Tape Reader
struct TapeReaderResult {
    int cumulativeDelta;             // net buying/selling pressure
    bool largeBlockDetected;         // institutional order spotted
    int blockSize;                   // size of the block (0 if none) 
    BookSide blockSide;              // BID or ASK 
    bool divergenceDetected;         // price vs delta disagreement
    bool isBearishDivergence;        // true = price up, delta down
};

// Signal strength levels
enum class SignalStrength { HIGH, MODERATE, LOW };

// Signal direction
enum class SignalDirection { BULLISH, BEARISH, NEUTRAL };

// Signal Aggregator — composite score from all 6 analyzers
struct AggregatedSignal {
    int score;                           // 0-6 composite score
    SignalStrength strength;             // HIGH (5-6), MODERATE (3-4), LOW (0-2)
    SignalDirection direction;           // overall bias from the signals
    // Individual factor scores (1 = contributing, 0 = not)
    int cprScore;                        // +1 if CPR bias aligned
    int camarillaScore;                  // +1 if at Camarilla level
    int vpaScore;                        // +1 if VPA confirming
    int bookFlipScore;                   // +1 if book flip detected
    int absorptionScore;                 // +1 if absorption detected
    int stackingScore;                   // +1 if stacking aligned
    // TapeReader confidence modifiers (not scored, but inform conviction)
    int cumulativeDelta;                 // net flow context
    bool largeBlockPresent;              // institutional activity
    bool divergenceWarning;              // price/delta disagree — reduce conviction
};

