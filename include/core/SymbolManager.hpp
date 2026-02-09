#pragma once

#include "core/SymbolState.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "core/IMarketDataListener.hpp"

class SymbolManager : public IMarketDataListener {
    public:
        void addSymbol(const std::string& symbol, double price = 0.0, int score = 0);
        void removeSymbol(const std::string& symbol);
        std::vector<SymbolState> getAllSymbols() const;
        bool hasSymbol(const std::string& symbol) const;
        int getSymbolCount() const;

        // IMarketDataListener implementation
        void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) override;
        void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) override;
        void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) override;

    private:
        void updatePrice(const std::string& symbol, double price);  // Internal helper
        std::unordered_map<std::string, SymbolState> symbols_;
        mutable std::mutex mutex_; 
}; 

