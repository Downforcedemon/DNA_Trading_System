#pragma once
#include <string>
#include <chrono>

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
};

