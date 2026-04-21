#include "strategy/StrategyEngine.hpp"
#include <ranges>

StrategyEngine::StrategyEngine(SymbolManager& symbolManager)
    : symbolManager_(symbolManager)
    , aggregator_() {
}

// Get or create per-symbol analyzers (lazy initialization)
// First time a symbol is seen, all 4 stateful analyzers are created with defaults
SymbolAnalyzers& StrategyEngine::getAnalyzers(const std::string& symbol) {
    auto it = analyzers_.find(symbol);
    if (it == analyzers_.end()) {
        // operator[] default-constructs SymbolAnalyzers in-place
        // (emplace fails because TapeReader's mutex is non-copyable)
        analyzers_[symbol];
        it = analyzers_.find(symbol);
    }
    return it->second;
}

// --- IMarketDataListener callbacks ---

// Price updates from IBKR Level 1
void StrategyEngine::onPriceUpdate(const std::string& symbol, double price, time_t timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Price is stored in SymbolManager by SymbolManager's own onPriceUpdate
    // StrategyEngine doesn't need to do anything here — analysis is driven by
    // onBookUpdate (Level 2) and onTradeUpdate (Time & Sales)
}

// Size updates from IBKR Level 1
void StrategyEngine::onSizeUpdate(const std::string& symbol, int size, time_t timestamp) {
    // Size alone isn't actionable — we need it paired with price in onTradeUpdate
}

// Error handling
void StrategyEngine::onError(const std::string& symbol, int errorCode, const std::string& errorMsg) {
    // Log errors but don't crash — market data can be noisy
}

// --- Core data entry points ---

// Called when Level 2 order book snapshot arrives
// Feeds: BookFlipDetector, StackingAnalyzer
// Caches results for runPipeline
void StrategyEngine::onBookUpdate(const std::string& symbol,
                                   const std::vector<PriceLevel>& bids,
                                   const std::vector<PriceLevel>& asks) {
    std::lock_guard<std::mutex> lock(mutex_);

    SymbolAnalyzers& sa = getAnalyzers(symbol);

    // BookFlip: detect significant size changes between snapshots → cache result
    sa.lastFlipResult = sa.bookFlip.detect(bids, asks);

    // Stacking: calculate bid/ask imbalance ratio (stateless) → cache result
    sa.lastStackingRatio = StackingAnalyzer::calculateRatio(bids, asks);

    // Store latest bid/ask for aggressor classification in onTradeUpdate
    if (!bids.empty()) lastBid_[symbol] = bids[0].price;
    if (!asks.empty()) lastAsk_[symbol] = asks[0].price;

    // Run pipeline after book update too — BookFlip/Stacking may change the score
    runPipeline(symbol);
}

// Called when a Time & Sales tick arrives
// Feeds: TapeReader, VPAAnalyzer
// Caches results, then runs full pipeline
void StrategyEngine::onTradeUpdate(const std::string& symbol, double price, int size,
                                    BookSide aggressor, double bid, double ask) {
    std::lock_guard<std::mutex> lock(mutex_);

    SymbolAnalyzers& sa = getAnalyzers(symbol);

    // TapeReader: cumulative delta, block detection, divergence → cache result
    sa.lastTapeResult = sa.tapeReader.processTrade(price, size, aggressor, bid, ask);

    // VPA: volume confirms or diverges from price → cache result
    sa.lastVpaResult = sa.vpa.detect(price, size);

    // Run the full scoring pipeline
    runPipeline(symbol);
}

// Set yesterday's OHLC for pivot calculations (called once at session start)
void StrategyEngine::setDailyOHLC(const std::string& symbol, const OHLCData& ohlc) {
    std::lock_guard<std::mutex> lock(mutex_);
    dailyOHLC_[symbol] = ohlc;
}

// Wired into the listener chain — fires when IBKRConnection::historicalDataEnd completes
void StrategyEngine::onOHLCUpdate(const std::string& symbol, const OHLCData& ohlc) {
    setDailyOHLC(symbol, ohlc);
}

// Reset all analyzers for new trading session (call at RTH open 9:30 ET)
void StrategyEngine::resetSession() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sa : analyzers_ | std::ranges::views::values) {
        sa.tapeReader.resetDelta();
        // Reset cached results to defaults
        sa.lastFlipResult = {false, FlipType::NONE, BookSide::NONE, 0.0, 0};
        sa.lastAbsorptionResult = {false, BookSide::NONE, 0.0, 0, 0};
        sa.lastVpaResult = {false, false, 0.0, 0};
        sa.lastTapeResult = {0, false, 0, BookSide::NONE, false, false};
        sa.lastStackingRatio = 1.0;
    }
}

// --- The Pipeline: where everything comes together ---

void StrategyEngine::runPipeline(const std::string& symbol) {
    // This runs INSIDE the lock from onTradeUpdate/onBookUpdate

    SymbolAnalyzers& sa = getAnalyzers(symbol);

    // Get current price from SymbolManager
    double currentPrice = symbolManager_.getPrice(symbol);
    if (currentPrice <= 0.0) return;  // no price yet, can't score

    // Calculate pivot levels (stateless — just needs OHLC)
    CPRData cpr = {0.0, 0.0, 0.0};
    CamarillaLevels camarilla = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    auto ohlcIt = dailyOHLC_.find(symbol);
    if (ohlcIt != dailyOHLC_.end()) {
        cpr = CPRCalculator::calculate(ohlcIt->second);
        camarilla = CamarillaCalculator::calculate(ohlcIt->second);
    }

    // Aggregate all cached results into composite score
    AggregatedSignal signal = aggregator_.aggregate(
        currentPrice, cpr, camarilla,
        sa.lastVpaResult,
        sa.lastFlipResult,
        sa.lastAbsorptionResult,
        sa.lastStackingRatio,
        sa.lastTapeResult
    );

    // Write scores back to SymbolManager so the dashboard shows them
    symbolManager_.updateSignalScore(symbol, signal.score);
    symbolManager_.updateFactorScores(symbol,
        signal.cprScore, signal.camarillaScore, signal.vpaScore,
        signal.bookFlipScore, signal.absorptionScore, signal.stackingScore);

    // Determine data confidence status for each factor based on how recently we've received updates
    bool hasOHLC = (ohlcIt != dailyOHLC_.end());
    bool hasL2 = (sa.lastFlipResult.detected || sa.lastStackingRatio != 1.0);
    bool hasTape = (sa.lastTapeResult.cumulativeDelta != 0 || sa.lastVpaResult.confirmed);

    symbolManager_.updateDataStatus(symbol,
        hasOHLC ? DataStatus::ACTIVE : DataStatus::NONE,              // CPR confidence depends on having OHLC for pivot calculation
        hasOHLC ? DataStatus::ACTIVE : DataStatus::NONE,              // Camarilla confidence also depends on OHLC
        hasTape ? DataStatus::ACTIVE : DataStatus::NONE,              // VPA confidence depends on having recent trade data
        hasL2 ? DataStatus::ACTIVE : DataStatus::NONE,                // BookFlip confidence depends on having recent L2 data
        hasTape ? DataStatus::ACTIVE : DataStatus::NONE,              // Absorption confidence depends on having recent trade data
        hasL2 ? DataStatus::ACTIVE : DataStatus::NONE);               // Stacking confidence depends on having recent L2 data
}
