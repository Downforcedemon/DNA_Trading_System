#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include "strategy/StrategyTypes.hpp"

enum class SignalType {
    BUY,
    SELL,
    WAIT
};

// Data confidence: does the analyzer have enough data to be meaningful?
enum class DataStatus {
    NONE,
    STALE,            // had data but hasn't updated recently
    ACTIVE            // receiving data, score is trustworthy
};

struct SymbolState {
    std::string symbol = "";
    std::string primaryExchange = "NASDAQ";  // L2 routing: NASDAQ | NYSE | ARCA | BATS | AMEX
    double currentPrice = 0.0;
    int signalScore = 0;
    SignalType signalType = SignalType::WAIT;
    std::string dataSource = "";
    time_t lastUpdate = 0;

    // Strategy fields - CPR and Camarilla levels
    CPRData cprLevels = {0.0,0.0,0.0};
    CamarillaLevels camarillaLevels = {0.0,0.0,0.0,0.0,0.0,0.0};

    // Type reading state
    double cumulativeDelta = 0.0;

    // Individual factor scores ( 0 or 1 each)
    int cprScore = 0; 
    int camarillaScore = 0;
    int vpaScore = 0;
    int bookFlipScore = 0;
    int absorptionScore = 0;
    int stackingScore = 0 ; 

    // Data confidence status
    DataStatus cprStatus = DataStatus::NONE;
    DataStatus camarillaStatus = DataStatus::NONE;
    DataStatus vpaStatus = DataStatus::NONE;
    DataStatus bookFlipStatus = DataStatus::NONE;
    DataStatus absorptionStatus = DataStatus::NONE;
    DataStatus stackingStatus = DataStatus::NONE;
};

