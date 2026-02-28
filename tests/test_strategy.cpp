#include <iostream>
#include <vector>
#include <string>
#include "core/SymbolManager.hpp"
#include "strategy/StrategyEngine.hpp"

// Helper to print signal details
void printSignal(const std::string& label, const SymbolManager& mgr) {
    auto symbols = mgr.getAllSymbols();
    for (const auto& sym : symbols) {
        std::string signalStr;
        if (sym.signalType == SignalType::BUY)       signalStr = "BUY";
        else if (sym.signalType == SignalType::SELL)  signalStr = "SELL";
        else                                          signalStr = "WAIT";

        std::cout << "[" << label << "] " << sym.symbol
                  << "  Score: " << sym.signalScore << "/6  Signal: " << signalStr
                  << "  | CPR:" << sym.cprScore
                  << " CAM:" << sym.camarillaScore
                  << " VPA:" << sym.vpaScore
                  << " FLIP:" << sym.bookFlipScore
                  << " ABS:" << sym.absorptionScore
                  << " STACK:" << sym.stackingScore
                  << std::endl;
    }
}

int main() {
    std::cout << "=== DNA Trading System — Strategy Pipeline Smoke Test ===" << std::endl;
    std::cout << std::endl;

    // --- Setup ---
    SymbolManager mgr;
    StrategyEngine engine(mgr);

    mgr.addSymbol("NVDA", 130.0, 0);
    mgr.addSymbol("AAPL", 185.0, 0);

    std::cout << std::endl;

    // --- Test 1: Set OHLC and verify CPR/Camarilla scoring ---
    std::cout << "--- Test 1: OHLC + Pivot Scoring ---" << std::endl;

    // NVDA: yesterday's candle — wide range day
    OHLCData nvdaOhlc = {125.0, 135.0, 120.0, 132.0, 0};
    engine.setDailyOHLC("NVDA", nvdaOhlc);

    // AAPL: yesterday's candle — narrow range
    OHLCData aaplOhlc = {183.0, 187.0, 182.0, 185.0, 0};
    engine.setDailyOHLC("AAPL", aaplOhlc);

    // Feed a trade to trigger pipeline (price above TC should score CPR +1)
    // NVDA pivot = (135+120+132)/3 = 129.0, BC = (135+120)/2 = 127.5, TC = 2*129 - 127.5 = 130.5
    // Price 133.0 is ABOVE TC (130.5) → CPR should score +1
    engine.onTradeUpdate("NVDA", 133.0, 100, BookSide::ASK, 132.9, 133.1);
    printSignal("After OHLC + trade", mgr);

    std::cout << std::endl;

    // --- Test 2: Book Update — Stacking ---
    std::cout << "--- Test 2: Stacking (heavy bid side) ---" << std::endl;

    // Heavy bid stacking: bid total = 5000, ask total = 1000 → ratio = 5.0 (bullish)
    std::vector<PriceLevel> bids = {
        {132.90, 2000}, {132.80, 1500}, {132.70, 1000}, {132.60, 500}
    };
    std::vector<PriceLevel> asks = {
        {133.10, 300}, {133.20, 300}, {133.30, 200}, {133.40, 200}
    };
    engine.onBookUpdate("NVDA", bids, asks);
    printSignal("After stacking", mgr);

    std::cout << std::endl;

    // --- Test 3: Feed many trades to warm up VPA + TapeReader ---
    std::cout << "--- Test 3: Warm up VPA + TapeReader (100 trades) ---" << std::endl;

    // Feed 100 small trades to get past minObservations
    for (int i = 0; i < 100; i++) {
        double price = 133.0 + (i % 5) * 0.01;  // small price movement
        engine.onTradeUpdate("NVDA", price, 50, BookSide::ASK, 132.9, 133.1);
    }
    printSignal("After warmup", mgr);

    std::cout << std::endl;

    // --- Test 4: Big buy-side trade (should trigger VPA + large block) ---
    std::cout << "--- Test 4: Large block trade (5000 shares, buy-side) ---" << std::endl;

    engine.onTradeUpdate("NVDA", 133.50, 5000, BookSide::ASK, 133.4, 133.6);
    printSignal("After large block", mgr);

    std::cout << std::endl;

    // --- Test 5: Reset session ---
    std::cout << "--- Test 5: Session reset ---" << std::endl;

    engine.resetSession();
    // Feed one trade after reset to trigger pipeline
    engine.onTradeUpdate("NVDA", 133.50, 100, BookSide::ASK, 133.4, 133.6);
    printSignal("After reset", mgr);

    std::cout << std::endl;

    // --- Test 6: AAPL — no data fed, should stay at 0 ---
    std::cout << "--- Test 6: AAPL (no trades fed, only OHLC set) ---" << std::endl;
    // Feed one trade to AAPL to trigger its pipeline
    engine.onTradeUpdate("AAPL", 185.0, 100, BookSide::BID, 184.9, 185.1);
    printSignal("AAPL first trade", mgr);

    std::cout << std::endl;
    std::cout << "=== Smoke Test Complete ===" << std::endl;

    return 0;
}
