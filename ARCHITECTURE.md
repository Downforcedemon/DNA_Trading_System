# DNA Trading System - Architecture Overview

## System Architecture Diagram (Phase 2: Provider-Agnostic)

```
┌─────────────────────────────────────────────────────────────────────┐
│                          DNA Trading System                         │
│                              (main.cpp)                             │
└──────────────────┬──────────────────────────────────────────────────┘
                   │
                   │
        ┌──────────▼─────────────────────────────┐
        │      MarketDataManager                 │
        │      (Observer Pattern Hub)            │
        │  ┌──────────────────────────────────┐  │
        │  │ IMarketDataProvider* provider_   │  │
        │  │ vector<IMarketDataListener*>     │  │
        │  └──────────────────────────────────┘  │
        └─────────┬────────────────────┬──────────┘
                  │                    │
     ┌────────────▼──────────┐    ┌────▼────────────────┐
     │   SymbolManager       │    │  StrategyEngine     │
     │   (IMarketDataListener)│    │  (IMarketDataListener)│
     │   (Thread-Safe)       │    │  [Phase 3 - Planned] │
     └───────────────────────┘    └─────────────────────┘
                  │
        ┌─────────▼──────────┐
        │   SymbolState      │
        │   (Data Structure) │
        └────────────────────┘

Provider Layer (Adapter Pattern):
┌──────────────────────────────────────────────────────┐
│              IBKRAdapter                             │
│  (IMarketDataProvider + IMarketDataListener)        │
│  ┌────────────────────────────────────────────────┐ │
│  │  Wraps IBKRConnection                          │ │
│  │  Implements provider interface                 │ │
│  │  Forwards callbacks to MarketDataManager      │ │
│  └────────────────────────────────────────────────┘ │
└──────────────────┬───────────────────────────────────┘
                   │
        ┌──────────▼──────────┐
        │  IBKRConnection     │
        │  (Legacy Wrapper)   │
        └──────────┬──────────┘
                   │
        ┌──────────▼──────────┐
        │  IBKR C++ API       │
        │  (EWrapper+EReader) │
        └──────────┬──────────┘
                   │
        ┌──────────▼──────────┐
        │   IB Gateway        │
        │   (Port 4001)       │
        └─────────────────────┘

External Libraries:
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  simdjson    │  │   protobuf   │  │  Intel DFP   │
│  (JSON)      │  │  (API msgs)  │  │  (Decimals)  │
└──────────────┘  └──────────────┘  └──────────────┘
```

---

## File Relationships and Data Flow

### 1. **Entry Point: `src/main.cpp`**

**Purpose:** Main program orchestration and user interface

**Responsibilities:**
- Creates `SymbolManager` instance
- Creates `IBKRConnection` instance
- Loads default symbols from JSON config
- Connects to IB Gateway
- Subscribes to market data for all symbols
- Runs interactive command loop
- Displays watchlist dashboard

**Key Code Flow:**
```cpp
main()
  → loadDefaultSymbols(manager)        // Uses simdjson
  → IBKRConnection ibkr(manager)       // Create API wrapper
  → ibkr.connect("127.0.0.1", 4001, 1) // Connect to Gateway
  → ibkr.subscribeMarketData(symbol)   // For each symbol
  → while(true) { handle commands }    // Interactive loop
```

**Dependencies:**
- `SymbolManager.hpp` / `SymbolManager.cpp`
- `IBKRConnection.hpp` / `IBKRConnection.cpp`
- `simdjson.h` / `simdjson.cpp`

---

### 2. **Data Model: `include/core/SymbolState.hpp`**

**Purpose:** Define the data structure for each stock symbol

**What it contains:**
```cpp
enum class SignalType {
    BUY,   // Algorithm says BUY
    SELL,  // Algorithm says SELL
    WAIT   // Algorithm says WAIT (neutral)
};

struct SymbolState {
    std::string symbol;        // e.g., "NVDA"
    double currentPrice;       // e.g., 145.23
    int signalScore;           // e.g., 5 (out of 6)
    SignalType signalType;     // BUY, SELL, or WAIT
};
```

**Used by:**
- `SymbolManager` (stores map of these)
- `main.cpp` (displays these in dashboard)

**No dependencies** - Pure data structure

---

### 3. **Storage Manager: `include/core/SymbolManager.hpp` + `src/core/SymbolManager.cpp`**

**Purpose:** Thread-safe storage and management of all symbols

**Data Structure:**
```cpp
class SymbolManager {
private:
    std::unordered_map<std::string, SymbolState> symbols_;  // O(1) lookup
    mutable std::mutex mutex_;                              // Thread safety

public:
    void addSymbol(symbol, price, score);
    void removeSymbol(symbol);
    void updatePrice(symbol, price);        // ← Called by IBKR callbacks
    std::vector<SymbolState> getAllSymbols();
    bool hasSymbol(symbol);
    int getSymbolCount();
};
```

**Key Features:**
- **O(1) lookups** using `unordered_map` (hash table)
- **Thread-safe** with `std::mutex` for concurrent updates
- **Case-insensitive** symbol handling (converts to uppercase)

**Data Flow:**
```
User adds symbol → addSymbol() → symbols_["NVDA"] = {symbol, price, score}
IBKR price tick  → updatePrice() → symbols_["NVDA"].currentPrice = 145.23
User types "list" → getAllSymbols() → returns vector of all SymbolStates
```

**Used by:**
- `main.cpp` (adds/removes symbols, displays list)
- `IBKRConnection.cpp` (updates prices from market data)

**Dependencies:**
- `SymbolState.hpp`

---

### 4. **API Wrapper: `include/core/IBKRConnection.hpp` + `src/core/IBKRConnection.cpp`**

**Purpose:** Wrap IBKR C++ API and connect to IB Gateway

**Class Structure:**
```cpp
class IBKRConnection : public DefaultEWrapper {
private:
    SymbolManager& symbolManager_;                    // Reference to manager
    std::unique_ptr<EClientSocket> client_;           // IBKR API client
    int nextOrderId_;                                 // From IBKR
    int nextTickerId_;                                // Auto-increment
    std::unordered_map<int, std::string> tickerIdToSymbol_;  // Maps ticker → symbol

public:
    bool connect(host, port, clientId);
    void disconnect();
    void subscribeMarketData(symbol);
    void processMessages();  // TODO: Implement with EReader

    // IBKR callbacks (inherited from EWrapper):
    void tickPrice(tickerId, field, price, attrib) override;
    void tickSize(tickerId, field, size) override;
    void error(id, errorCode, errorString) override;
    void nextValidId(orderId) override;
    void connectionClosed() override;
};
```

**Data Flow - Market Data Subscription:**
```
main.cpp calls:
  ibkr.subscribeMarketData("NVDA")
    → nextTickerId_++ = 5
    → tickerIdToSymbol_[5] = "NVDA"
    → client_->reqMktData(5, contract, ...)  // IBKR API call
    → Sends request to IB Gateway
```

**Data Flow - Price Update:**
```
IB Gateway sends price tick
  → IBKR API calls: tickPrice(tickerId=5, field=4, price=145.23, ...)
    → Look up symbol: tickerIdToSymbol_[5] = "NVDA"
    → Update manager: symbolManager_.updatePrice("NVDA", 145.23)
    → Print: "💰 NVDA: $145.23"
```

**Key Features:**
- Inherits from `DefaultEWrapper` (IBKR base class)
- Maps ticker IDs to symbol names (IBKR uses numeric IDs)
- Thread-safe updates through `SymbolManager`'s mutex
- Handles connection lifecycle

**Dependencies:**
- `SymbolManager.hpp`
- IBKR API headers (`EWrapper.h`, `EClientSocket.h`, `DefaultEWrapper.h`, etc.)

---

### 5. **Configuration: `config/symbols.json`**

**Purpose:** Default watchlist loaded at startup

**Format:**
```json
{
    "watchlist": ["NVDA", "AAPL", "TSLA", "AMD", "MSFT"]
}
```

**How it's used:**
```cpp
// In main.cpp:
void loadDefaultSymbols(SymbolManager& manager) {
    std::ifstream file("config/symbols.json");
    simdjson::padded_string json_str(...);
    simdjson::ondemand::document doc = parser.iterate(json);
    auto watchlist = doc["watchlist"];

    for (auto symbol : watchlist) {
        std::string sym = std::string(symbol.get_string().value());
        manager.addSymbol(sym, 100.0, 3);  // Default price/score
    }
}
```

---

### 6. **Build System: `CMakeLists.txt`**

**Purpose:** Compile and link everything together

**Key Configuration:**
```cmake
# C++23 standard
set(CMAKE_CXX_STANDARD 23)

# Include paths
include_directories(include)
include_directories(external)
include_directories(external/ibkr)
include_directories(external/ibkr/protobufUnix)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
set(SOURCES ${SOURCES} external/simdjson.cpp)
file(GLOB IBKR_SOURCES "external/ibkr/*.cpp")
file(GLOB IBKR_PROTOBUF_SOURCES "external/ibkr/protobufUnix/*.cc")

# Create executable
add_executable(dna_system ${SOURCES})

# Link libraries
target_link_libraries(dna_system
    pthread                              # Threading
    ${Protobuf_LIBRARIES}               # Protobuf
    ${PROJECT_SOURCE_DIR}/external/libbid.a  # Intel DFP
)
```

---

## Complete Data Flow Example

### Scenario: User adds NVDA and receives price update

```
1. Program Start
   main.cpp → loadDefaultSymbols()
           → SymbolManager.addSymbol("NVDA", 100.0, 3)
           → symbols_["NVDA"] = {symbol:"NVDA", price:100.0, score:3, signal:WAIT}

2. Connect to IBKR
   main.cpp → ibkr.connect("127.0.0.1", 4001, 1)
           → EClientSocket connects to IB Gateway
           → IB Gateway calls nextValidId(orderId)

3. Subscribe to Market Data
   main.cpp → ibkr.subscribeMarketData("NVDA")
           → nextTickerId_++ = 5
           → tickerIdToSymbol_[5] = "NVDA"
           → client_->reqMktData(5, contract, "")
           → IB Gateway starts streaming ticks

4. Price Update (async callback)
   IB Gateway sends: tickPrice(tickerId=5, field=4, price=145.23)
                  → tickPrice() callback fires
                  → symbol = tickerIdToSymbol_[5] = "NVDA"
                  → symbolManager_.updatePrice("NVDA", 145.23)
                  → mutex locks
                  → symbols_["NVDA"].currentPrice = 145.23
                  → mutex unlocks
                  → Print: "💰 NVDA: $145.23"

5. User Types "list"
   main.cpp → manager.getAllSymbols()
           → Returns vector of all SymbolStates
           → Displays formatted table:
              SYMBOL  | PRICE    | SIGNAL | SCORE
              NVDA    | $145.23  | - WAIT | 3/6
```

---

## Thread Safety Architecture

```
Main Thread                    IBKR Callback Thread
-----------                    --------------------
User input                     tickPrice() called
  ↓                                 ↓
manager.addSymbol()            manager.updatePrice()
  ↓                                 ↓
mutex_.lock()                  mutex_.lock()
symbols_[...] = ...            symbols_[...].price = ...
mutex_.unlock()                mutex_.unlock()
```

**Why thread-safe?**
- IBKR callbacks run on different thread
- Both threads modify `symbols_` map
- `std::mutex` prevents race conditions

---

## Current Limitations & Next Steps

### ✅ What's Working
- All files compile and link successfully
- Connection to IB Gateway established
- Market data subscriptions active
- Thread-safe price storage ready
- Interactive dashboard functional

### ⚠️ What's Not Working Yet
1. **EReader not implemented**
   - `processMessages()` is currently empty
   - Need to create EReader thread for async message processing

2. **Signals are hardcoded**
   - All symbols show WAIT with score 3/6
   - Need to implement 6-factor scoring algorithm

3. **Not tested with live data**
   - Markets closed (Sunday)
   - Monday testing will verify price updates flow correctly

### 📋 Next Implementation Tasks
1. Implement EReader in `IBKRConnection.cpp`
2. Test live market data on Monday
3. Build signal scoring system
4. Add more market data fields (bid/ask, volume)

---

## Key Design Decisions

### 1. **Why `unordered_map` instead of `vector`?**
- O(1) lookup by symbol name vs O(n) linear search
- Important for HFT performance

### 2. **Why separate SymbolManager from IBKRConnection?**
- **Separation of concerns:** Storage vs API communication
- **Testability:** Can test SymbolManager without IBKR
- **Thread safety:** Centralized lock in one class

### 3. **Why smart pointers for EClientSocket?**
- **RAII:** Automatic cleanup on destruction
- **Exception safety:** No memory leaks if connection fails

### 4. **Why reference to SymbolManager in IBKRConnection?**
- **Single source of truth:** One SymbolManager instance
- **No copying:** Pass by reference is efficient
- **Lifetime management:** main.cpp owns it, IBKRConnection uses it

### 5. **Why static library (libbid.a) instead of dynamic?**
- **Portability:** No runtime dependencies
- **Simplicity:** Single executable file
- **Performance:** Can be optimized at link time

---

## Phase 3: Tape Reading Strategy Architecture (Planned)

### Overview
Port proven Python tape reading/order flow analysis system with 6-factor composite scoring.

### Strategy Components

```
┌─────────────────────────────────────────────────────────────┐
│                    StrategyEngine                           │
│              (IMarketDataListener)                          │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              TapeReader                              │  │
│  │  - Cumulative Delta Tracking                         │  │
│  │  - Large Block Detection                             │  │
│  │  - Delta Divergence Detection                        │  │
│  │  - Trade Velocity Calculation                        │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌────────────────── 6 Analyzers ──────────────────────┐  │
│  │                                                       │  │
│  │  BookFlipDetector    AbsorptionDetector             │  │
│  │  (Order appearance)   (Volume at levels)             │  │
│  │                                                       │  │
│  │  StackingAnalyzer    CamarillaCalculator            │  │
│  │  (Bid/ask imbalance) (Pivot levels)                  │  │
│  │                                                       │  │
│  │  CPRCalculator       VPAAnalyzer                     │  │
│  │  (Central pivots)    (Volume patterns)               │  │
│  │                                                       │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │                                  │
│  ┌───────────────────────▼──────────────────────────────┐  │
│  │           SignalAggregator                           │  │
│  │  - Combines 6 factors into composite score (0-6)    │  │
│  │  - Determines BUY/SELL/WAIT signal                   │  │
│  │  - Calculates confidence level                       │  │
│  └──────────────────────┬───────────────────────────────┘  │
│                         │                                   │
└─────────────────────────┼───────────────────────────────────┘
                          │
                          ▼
                  Update SymbolState
            (score, signal, reason, components)
```

### 6-Factor Scoring System

Each factor awards 0 or 1 point:

1. **CPR Bias** - Is price above/below Central Pivot Range?
2. **Camarilla Level** - Is price at key H3/L3/H4/L4 level?
3. **VPA Signal** - Does volume confirm price movement?
4. **Book Flip** - Did large order just appear/disappear?
5. **Absorption** - Is volume being absorbed at support/resistance?
6. **Stacking** - Is order book imbalanced >70% or <30%?

**Signal Interpretation:**
- 5-6 points = HIGH probability (take trade)
- 3-4 points = MODERATE (wait for confirmation)
- 0-2 points = LOW (no trade)

### Enhanced SymbolState

**Existing Fields:**
- Symbol identifier, current price, signal type, signal score

**New Fields Needed:**
- Level 2 Order Book (bids, asks, sizes, stacking ratio)
- Time & Sales Tape (recent trades, buy/sell volume, cumulative delta)
- Detection Flags (book flip, absorption indicators)
- Technical Indicators (Camarilla levels, CPR data, VPA signals)
- Signal Details (reasoning, timestamp)

### Data Flow - Strategy Processing

```
Market Data Update
    ↓
MarketDataManager broadcasts to listeners
    ↓
StrategyEngine.onPriceUpdate(symbol, price, timestamp)
    ↓
Process through analyzers (parallel):
    - BookFlipDetector checks order book changes
    - AbsorptionDetector tracks volume at levels
    - StackingAnalyzer calculates bid/ask ratio
    - CamarillaCalculator computes pivot levels
    - CPRCalculator computes central pivots
    - VPAAnalyzer analyzes volume patterns
    ↓
TapeReader processes:
    - Updates cumulative delta
    - Detects large blocks
    - Checks for divergences
    - Calculates velocity
    ↓
SignalAggregator combines:
    - Collects all 6 factor results
    - Counts bullish vs bearish signals
    - Calculates composite score (0-6)
    - Determines signal type (BUY/SELL/WAIT)
    - Generates reasoning text
    ↓
Update SymbolState with results
    ↓
Dashboard displays signal score and reasoning
```

### Implementation Phases

**Phase 3.1:** Foundation Types
- Create StrategyTypes.hpp (Trade, PriceLevel, FootprintBar)
- Enhance SymbolState with new fields

**Phase 3.2:** Simple Analyzers
- StackingAnalyzer (ratio calculations)
- CamarillaCalculator (math formulas)
- CPRCalculator (math formulas)

**Phase 3.3:** Stateful Analyzers
- BookFlipDetector (tracks book state)
- AbsorptionDetector (tracks price levels)
- VPAAnalyzer (pattern recognition)

**Phase 3.4:** Tape Reader
- Cumulative delta tracking
- Large block detection
- Delta divergence detection

**Phase 3.5:** Signal Aggregator
- 6-factor composite scoring
- Signal interpretation logic

**Phase 3.6:** Integration
- Create StrategyEngine class
- Wire all components together
- Register as MarketDataManager listener

**Phase 3.7:** Testing & Validation
- Unit tests per analyzer
- Integration tests with live data
- Performance profiling

### Data Requirements

**From IBKR API:**
- Level 2 Market Depth (reqMktDepth)
- Time & Sales Tick Data (reqTickByTickData)
- Historical OHLC for pivots (reqHistoricalData)

**Per Symbol:**
- Float shares (for dynamic block thresholds)
- Previous day OHLC (for Camarilla/CPR)
- Support/resistance levels (manual or auto-detected)

### Thread Safety Considerations

- StrategyEngine shares mutex with SymbolManager
- All analyzer state is per-symbol (no shared state)
- Atomic updates to SymbolState
- Lock-free reads where possible (const methods)

### Performance Targets

- Signal calculation: <1ms per symbol
- Memory per symbol: <1MB (circular buffers)
- Support for 50+ simultaneous symbols
- Real-time processing of 100+ ticks/second per symbol

1. CPR (Central Pivot Range) Calculation:

Input: Previous day High, Low, Close
Formulas:
Pivot = (H + L + C) / 3
BC = (H + L) / 2
TC = (Pivot - BC) + Pivot
Signal: Price > TC = Bullish (+1), Price < BC = Bearish (+1)

2. Camarilla Levels:

Input: Previous day H, L, C
Key levels:
R3 = C + ((H - L) × 1.1 / 4) - Resistance (reversal level)
S3 = C - ((H - L) × 1.1 / 4) - Support (reversal level)
R4 = C + ((H - L) × 1.1 / 2) - Breakout resistance
S4 = C - ((H - L) × 1.1 / 2) - Breakout support
R6 = C × H / L - Overdrive resistance (extreme high)
S6 = C - (R6 - C) - Overdrive support (extreme low)
Signal: Price within 0.2% of R3/S3/R4/S4 = +1 point

3. Stacking Analyzer:

Input: Level 2 order book (bids and asks)
Formula: Ratio = Total_Bid_Size / (Total_Bid_Size + Total_Ask_Size)
Signal: Ratio > 70% = Bullish (+1), Ratio < 30% = Bearish (+1)
What to Code First (Phase 3.1-3.2):
StrategyTypes.hpp - Define structs:

PriceLevel (price, size)
OHLCData (open, high, low, close)
CamarillaLevels (r3, r4, r6, s3, s4, s6)
CPRData (pivot, BC, TC)
Enhance SymbolState.hpp - Add fields for order book and indicators

CamarillaCalculator - Pure math class, no state

CPRCalculator - Pure math class, no state

StackingAnalyzer - Takes order book, returns ratio
