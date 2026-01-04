#pragma once

#include "SymbolState.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>


class SymbolManager {
    public:
        void addSymbol(const std::string& symbol, double price = 0.0, int score = 0);
        void removeSymbol(const std::string& symbol);
        std::vector<SymbolState> getAllSymbols() const;
        bool hasSymbol(const std::string& symbol) const;
        int getSymbolCount() const;
        void updatePrice(const std::string& symbol, double price);

    private:
        std::unordered_map<std::string, SymbolState> symbols_;
        mutable std::mutex mutex_; 
}; 
