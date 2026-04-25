#include "core/SymbolManager.hpp"
#include <iostream>
#include <algorithm>

void SymbolManager::addSymbol(const std::string& symbol, const std::string& exchange, double price, int score) {
    if (symbols_.count(symbol) > 0) {
        std::cout << "⚠️  " << symbol << " already in watchlist" << std::endl;
        return;
    }

    SymbolState state;
    state.symbol = symbol;
    state.primaryExchange = exchange;
    state.currentPrice = price;
    state.signalScore = score;

    // signal type based on score (temporary logic)
    if (score >= 5) {
        state.signalType = SignalType::BUY;
    } else if (score <= 2){
        state.signalType = SignalType::SELL;
    } else {
        state.signalType = SignalType::WAIT;
    }

    symbols_[symbol] = state;
    std::cout << "✅ Added " << symbol << std::endl;
}

void SymbolManager::removeSymbol(const std::string& symbol) {
    if (symbols_.erase(symbol) > 0) {
        std::cout << "➖ Removed " << symbol << std::endl;
    } else {
        std::cout << "⚠️  " << symbol << " not found" << std::endl;
    }
}

std::vector<SymbolState> SymbolManager::getAllSymbols() const {
    std::vector<SymbolState> result;
    for (const auto& pair : symbols_) {
        result.push_back(pair.second);
    }
    return result;
}

bool SymbolManager::hasSymbol(const std::string& symbol) const {
    return symbols_.count(symbol) > 0;
}

int SymbolManager::getSymbolCount() const {
    return symbols_.size();
}

std::string SymbolManager::getExchange(const std::string& symbol) const {
    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        return it->second.primaryExchange;
    }
    return "NASDAQ";
}

void SymbolManager::updatePrice(const std::string& symbol, double price) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        it->second.currentPrice = price;
    }
}

// --- Strategy score methods (called by StrategyEngine) ---

double SymbolManager::getPrice(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        return it->second.currentPrice;
    }
    return 0.0;
}

void SymbolManager::updateSignalScore(const std::string& symbol, int score) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        it->second.signalScore = score;
        // Update signal type based on score
        if (score >= 5) {
            it->second.signalType = SignalType::BUY;
        } else if (score <= 2) {
            it->second.signalType = SignalType::SELL;
        } else {
            it->second.signalType = SignalType::WAIT;
        }
    }
}

void SymbolManager::updateFactorScores(const std::string& symbol,
    int cpr, int camarilla, int vpa, int bookFlip, int absorption, int stacking) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        it->second.cprScore = cpr;
        it->second.camarillaScore = camarilla;
        it->second.vpaScore = vpa;
        it->second.bookFlipScore = bookFlip;
        it->second.absorptionScore = absorption;
        it->second.stackingScore = stacking;
    }
}

// Update data confidence status for a specific factor
void SymbolManager::updateDataStatus(const std::string& symbol, 
    DataStatus cpr, DataStatus camarilla, DataStatus vpa, DataStatus bookFlip, DataStatus absorption, DataStatus stacking) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = symbols_.find(symbol);
        if (it != symbols_.end()){
            it ->second.cprStatus = cpr;
            it ->second.camarillaStatus = camarilla;
            it ->second.vpaStatus = vpa;
            it ->second.bookFlipStatus = bookFlip;
            it ->second.absorptionStatus = absorption;
            it ->second.stackingStatus = stacking;
        }
    }


// IMarketDataListener implementation
void SymbolManager::onPriceUpdate(const std::string& symbol, double price, time_t timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        it->second.currentPrice = price;
        it->second.lastUpdate = timestamp;
        // dataSource will be set by MarketDataManager based on active provider
    }
}

void SymbolManager::onSizeUpdate(const std::string& symbol, int size, time_t timestamp) {
    // For now, we don't store size/volume in SymbolState
    // Can be extended later if needed
}

void SymbolManager::onError(const std::string& symbol, int errorCode, const std::string& errorMsg) {
    std::cout << "⚠️  Error for " << symbol << " [" << errorCode << "]: " << errorMsg << std::endl;
}

void SymbolManager::onExchangeDiscovered(const std::string& symbol, const std::string& exchange) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = symbols_.find(symbol);
    if (it != symbols_.end()) {
        it->second.primaryExchange = exchange;
    }
}
