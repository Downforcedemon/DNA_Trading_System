#pragma once
#include "core/IMarketDataListener.hpp"
#include "core/SymbolManager.hpp"
#include "strategy/StrategyTypes.hpp"
#include "strategy/SignalAggregator.hpp"
#include "strategy/analyzers/BookFlipDetector.hpp"
#include "strategy/analyzers/AbsorptionDetector.hpp"
#include "strategy/analyzers/VPAAnalyser.hpp"
#include "strategy/analyzers/TapeReader.hpp"
#include "strategy/analyzers/CamarillaCalculator.hpp"
#include "strategy/analyzers/CPRCalculator.hpp"
#include "strategy/analyzers/StackingAnalyzer.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

// Per-symbol analyzer state — each symbol gets its own set of stateful analyzers
struct SymbolAnalyzers {
    BookFlipDetector bookFlip;
    AbsorptionDetector absorption;
    VPAAnalyzer vpa;
    TapeReader tapeReader;

    // Cached latest results (persisted between onBookUpdate and runPipeline)
    BookFlipResult lastFlipResult = {false, FlipType::NONE, BookSide::NONE, 0.0, 0};
    AbsorptionResult lastAbsorptionResult = {false, BookSide::NONE, 0.0, 0, 0};
    VPAResult lastVpaResult = {false, false, 0.0, 0};
    TapeReaderResult lastTapeResult = {0, false, 0, BookSide::NONE, false, false};
    double lastStackingRatio = 1.0;
};

class StrategyEngine : public IMarketDataListener {
    public:
        // Constructor: takes reference to SymbolManager to update scores
        StrategyEngine(SymbolManager& symbolManager);

        // IMarketDataListener callbacks — receives data from MarketDataManager
        void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) override;
        void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) override;
        void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) override;

        // Feed order book data (called when Level 2 updates arrive)
        void onBookUpdate(const std::string& symbol,
                          const std::vector<PriceLevel>& bids,
                          const std::vector<PriceLevel>& asks);

        // Feed trade data (called when Time & Sales tick arrives)
        void onTradeUpdate(const std::string& symbol, double price, int size,
                           BookSide aggressor, double bid, double ask);

        // Set daily OHLC for pivot calculations (called once at session start)
        void setDailyOHLC(const std::string& symbol, const OHLCData& ohlc);

        // IMarketDataListener — receives OHLC from MarketDataManager, feeds setDailyOHLC
        void onOHLCUpdate(const std::string& symbol, const OHLCData& ohlc) override;

        // Reset all analyzers for new trading day
        void resetSession();

    private:
        SymbolManager& symbolManager_;                              // updates dashboard scores
        SignalAggregator aggregator_;                                // scoring engine
        std::unordered_map<std::string, SymbolAnalyzers> analyzers_; // per-symbol analyzers
        std::unordered_map<std::string, OHLCData> dailyOHLC_;       // per-symbol daily candle
        std::unordered_map<std::string, double> lastBid_;           // latest bid per symbol
        std::unordered_map<std::string, double> lastAsk_;           // latest ask per symbol
        mutable std::mutex mutex_;

        // Get or create analyzers for a symbol
        SymbolAnalyzers& getAnalyzers(const std::string& symbol);

        // Run full signal pipeline and update SymbolState
        void runPipeline(const std::string& symbol);
};
