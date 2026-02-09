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

struct SymbolState {
    std::string symbol;
    double currentPrice;
    int signalScore;
    SignalType signalType;
    std::string dataSource;
    time_t lastUpdate;

    // Strategy fields - CPR and Camarilla levels
    CPRData cprLevels;
    CamarillaLevels camarillaLevels;

    // Type reading state
    double cumulativeDelta;

    // Individual factor scores ( 0 or 1 each)
    int cprScore; 
    int camarillaScore;
    int vpaScore;
    int bookFlipScore;
    int absorptionScore;
    int stackingScore; 
};

