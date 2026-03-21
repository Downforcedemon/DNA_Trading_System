#include <iostream>
#include <string>
#include <sstream>
#include <atomic>
#include <thread>
#include <chrono>
#include "core/SymbolManager.hpp"
#include <fstream>
#include "simdjson.h"
#include "core/MarketDataManager.hpp"
#include "adapters/IBKRAdapter.hpp"
#include "strategy/StrategyEngine.hpp"

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

// Load IBKR configuration from file
IBKRConfig loadIBKRConfig() {
    simdjson::ondemand::parser parser;

    std::ifstream file("config/ibkr.json");
    if (!file.is_open()) {
        std::cout << "⚠️  No IBKR config file found, using defaults" << std::endl;
        return {"127.0.0.1", 4001, 0};  // Default config
    }

    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    simdjson::padded_string json(json_str);
    simdjson::ondemand::document doc = parser.iterate(json);

    IBKRConfig config;
    config.host = std::string(doc["host"].get_string().value());
    config.port = int64_t(doc["port"].get_int64().value());
    config.clientId = int64_t(doc["clientId"].get_int64().value());

    return config;
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

    // Create market data manager and strategy engine
    MarketDataManager marketDataManager;
    marketDataManager.addListener(&manager);

    StrategyEngine strategyEngine(manager);
    marketDataManager.addListener(&strategyEngine);

    // Load IBKR configuration
    IBKRConfig ibkrConfig = loadIBKRConfig();
    std::cout << "📡 IBKR Config: " << ibkrConfig.host << ":" << ibkrConfig.port
              << " (Client ID: " << ibkrConfig.clientId << ")" << std::endl;

    // Create IBKR adapter and set as provider
    auto ibkrAdapter = std::make_unique<IBKRAdapter>(&marketDataManager, ibkrConfig);
    auto* provider = ibkrAdapter.get();
    marketDataManager.setProvider(std::move(ibkrAdapter));

    // Connect to IB Gateway
    if (provider->connect()) {
        std::cout << "Connected to " << provider->getProviderName() << std::endl;

        // Subscribe to market data for all symbols
        auto symbols = manager.getAllSymbols();
        for (const auto& sym : symbols) {
            provider->subscribe(sym.symbol);
        }
    } else {
        std::cout << "Failed to connect to IB Gateway - running in offline mode" << std::endl;
    }

    printHelp();

    // Background thread: continuously process IBKR messages
    std::atomic<bool> running{true};
    std::thread marketThread([&]() {
        while (running) {
            if (provider->isConnected()) {
                provider->processMessages();  // blocks up to signal timeout (2s)
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });

    // Main thread: handle user input
    std::string input;
    while (true) {
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
                provider->unsubscribe(sym.symbol);
                manager.removeSymbol(sym.symbol);
            }
            std::cout << "Cleared all symbols" << std::endl;
            continue;
        }

        // Remove symbol
        if (input[0] == '-'){
            std::string symbol = toUpper(input.substr(1));
            provider->unsubscribe(symbol);
            manager.removeSymbol(symbol);
            continue;
        }

        // Add symbol and subscribe to market data
        std::string upperSymbol = toUpper(input);
        manager.addSymbol(upperSymbol, 0.0, 0);
        if (provider->isConnected()) {
            provider->subscribe(upperSymbol);
        }
    }

    // Clean shutdown: stop market thread, then disconnect
    running = false;
    if (marketThread.joinable()) {
        marketThread.join();
    }

    return 0;
}
