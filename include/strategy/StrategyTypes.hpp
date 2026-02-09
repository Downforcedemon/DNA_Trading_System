#pragma once
#include <string>
#include <ctime>

// Represents a single price level in order book
struct PriceLevel {
    double price; 
    int size; 
} 

// OHLC Data for pivot calculations
struct OHLCData {
    double open; 
    double high; 
    double low; 
    double close; 
    time_ti timestamp;
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


