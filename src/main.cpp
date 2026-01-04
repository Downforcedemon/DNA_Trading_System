#include <iostream>
#include <string>
#include <sstream>
#include "core/SymbolManager.hpp"
#include <fstream>
#include "simdjson.h"
#include "core/IBKRConnection.hpp"

// make all symbols uppercase
std::string toUpper(std::string str){
    for (char& c : str){
        c = std::toupper(c);
    }
    return str;
}

void printWatchlist(const SymbolManager& manager) {
    auto symbols = manager.getAllSymbols();
    
    if (symbols.empty()) {
        std::cout << "\n📊 No symbols in watchlist" << std::endl;
        return;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "SYMBOL   | PRICE    | SIGNAL  | SCORE" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (const auto& sym : symbols) {
        std::string signalStr;
        if (sym.signalType == SignalType::BUY) {
            signalStr = "▲ BUY  ";
        } else if (sym.signalType == SignalType::SELL) {
            signalStr = "▼ SELL ";
        } else {
            signalStr = "— WAIT ";
        }
        
        printf("%-8s | $%-7.2f | %-7s | %d/6\n", 
               sym.symbol.c_str(), 
               sym.currentPrice, 
               signalStr.c_str(), 
               sym.signalScore);
    }
    std::cout << "========================================\n" << std::endl;
}


void printHelp() {
    std::cout << "\n=== COMMANDS ===" << std::endl;
    std::cout << "  NVDA              - Add symbol" << std::endl;
    std::cout << "  -NVDA             - Remove symbol" << std::endl;
    std::cout << "  list              - Show watchlist" << std::endl;
    std::cout << "  clear             - Clear all" << std::endl;
    std::cout << "  help              - Show commands" << std::endl;
    std::cout << "  q / quit          - Exit" << std::endl;
    std::cout << "================\n" << std::endl;
}

// Import symbols
void loadDefaultSymbols(SymbolManager& manager) {
    simdjson::ondemand::parser parser;
    
    std::ifstream file("config/symbols.json");
    if (!file.is_open()) {
        std::cout << "⚠️  No config file found, starting with empty watchlist" << std::endl;
        return;
    }
    
    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    
    simdjson::padded_string json(json_str);
    simdjson::ondemand::document doc = parser.iterate(json);
    
    auto watchlist = doc["watchlist"];
    
    for (auto symbol : watchlist) {
        std::string sym = std::string(symbol.get_string().value());
        manager.addSymbol(sym, 100.0, 3);  // Default price and score
    }
    
    std::cout << "✅ Loaded " << manager.getSymbolCount() << " symbols from config" << std::endl;
}

int main() {
    std::cout << "DNA Trading System v1.0" << std::endl;
    std::cout << "========================================\n" << std::endl;

    SymbolManager manager;
    loadDefaultSymbols(manager);

    // Connect to IB Gateway
    IBKRConnection ibkr(manager);
    if (ibkr.connect("127.0.0.1", 4002, 1)) {
        // Subscribe to market data for all symbols
        auto symbols = manager.getAllSymbols();
        for (const auto& sym : symbols) {
            ibkr.subscribeMarketData(sym.symbol);
        }
    } else {
        std::cout << "⚠️  Failed to connect to IB Gateway - running in offline mode" << std::endl;
    }

    printHelp();

    std::string input;
    while (true) {
        ibkr.processMessages();

        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        // Quit
        if (input == "q" || input == "quit") {
            std::cout << "Shutting down..." << std::endl;
            break;
        }
        
        // Help
        if (input == "help") {
            printHelp();
            continue;
        }
        
        // List
        if (input == "list") {
            printWatchlist(manager);
            continue;
        }
        
        // Clear
        if (input == "clear") {
            auto symbols = manager.getAllSymbols();
            for (const auto& sym : symbols) {
                manager.removeSymbol(sym.symbol);
            }
            std::cout << "✅ Cleared all symbols" << std::endl;
            continue;
        }
        
        // Remove symbol
        if (input[0] == '-'){
            std::string symbol = toUpper(input.substr(1));
            manager.removeSymbol(symbol);
            continue;
        }
        
        // Add symbol (for now with dummy data)
        std::string upperSymbol = toUpper(input);
        manager.addSymbol(upperSymbol, 100.0, 3);
    }
    
    return 0;
}

