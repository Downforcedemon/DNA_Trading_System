# DNA Trading System V2 — Architecture

## Vision

A unified trading platform that replaces ATAS, DeepCharts, Sierra Chart, and MT5 with a single cross-platform application. Three modes of operation — **Charting**, **Backtesting**, **Live Trading** — sharing one codebase, one data pipeline, and one strategy engine.

**Languages:** C++ (core engine, rendering, performance-critical paths) + Python (scripting, custom indicators, research/backtesting notebooks)

**Platforms:** Windows (primary), Linux (secondary) — same codebase, same CMake build

---

## High-Level System Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          DNA Trading System V2                              │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        PRESENTATION LAYER                          │   │
│  │                    (ImGui + ImPlot + OpenGL)                        │   │
│  │                                                                     │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │   │
│  │  │  Chart   │ │ Footprint│ │   DOM    │ │  Tape /  │ │ Order   │ │   │
│  │  │  Engine  │ │  Chart   │ │ Heatmap  │ │  T&S     │ │  Panel  │ │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └─────────┘ │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │   │
│  │  │  Volume  │ │ Drawing  │ │Indicator │ │Backtest  │ │  P&L /  │ │   │
│  │  │  Profile │ │  Tools   │ │  Overlay │ │ Results  │ │  Risk   │ │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └─────────┘ │   │
│  │                                                                     │   │
│  │  Layout Manager (docking, tabs, multi-monitor, workspaces)          │   │
│  └─────────────────────────────┬───────────────────────────────────────┘   │
│                                │                                           │
│  ┌─────────────────────────────▼───────────────────────────────────────┐   │
│  │                        APPLICATION LAYER                            │   │
│  │                                                                     │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌────────────────────────┐ │   │
│  │  │  Chart State   │  │  Backtest     │  │  Live Trading         │ │   │
│  │  │  Manager       │  │  Controller   │  │  Controller           │ │   │
│  │  │               │  │               │  │                        │ │   │
│  │  │ - timeframes  │  │ - replay      │  │ - order routing       │ │   │
│  │  │ - bar builders│  │ - sim engine  │  │ - position tracking   │ │   │
│  │  │ - indicators  │  │ - optimizer   │  │ - risk enforcement    │ │   │
│  │  │ - crosshair   │  │ - analytics   │  │ - execution reports   │ │   │
│  │  └───────────────┘  └───────────────┘  └────────────────────────┘ │   │
│  │                                                                     │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │              Strategy Engine (V1 — preserved)               │   │   │
│  │  │  6 Analyzers: CPR | Camarilla | VPA | BookFlip | Abs | Stk │   │   │
│  │  │  TapeReader → SignalAggregator → Composite Score (0-6)     │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  │                                                                     │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │              Python Scripting Bridge (pybind11)             │   │   │
│  │  │  Custom indicators | Strategy scripts | Research notebooks │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────┬───────────────────────────────────────┘   │
│                                │                                           │
│  ┌─────────────────────────────▼───────────────────────────────────────┐   │
│  │                          DATA LAYER                                 │   │
│  │                                                                     │   │
│  │  ┌───────────────────┐  ┌───────────────┐  ┌────────────────────┐ │   │
│  │  │  MarketDataManager│  │  Historical   │  │  Bar Builder       │ │   │
│  │  │  (V1 — preserved) │  │  Data Store   │  │  Pipeline          │ │   │
│  │  │                   │  │               │  │                    │ │   │
│  │  │  Observer pattern │  │  HDF5 / mmap  │  │  tick → time bars │ │   │
│  │  │  Provider-agnostic│  │  binary files │  │  tick → tick bars  │ │   │
│  │  │  L1 + L2 + T&S   │  │  daily cache  │  │  tick → range bars │ │   │
│  │  └───────────────────┘  └───────────────┘  │  tick → renko     │ │   │
│  │                                             │  tick → vol bars  │ │   │
│  │  ┌───────────────────────────────────────┐ └────────────────────┘ │   │
│  │  │         Data Feed Adapters            │                        │   │
│  │  │                                       │                        │   │
│  │  │  ┌──────┐ ┌────────┐ ┌─────────────┐│                        │   │
│  │  │  │ IBKR │ │Databento│ │  Polygon    ││                        │   │
│  │  │  │(V1)  │ │(future) │ │  (future)   ││                        │   │
│  │  │  └──────┘ └────────┘ └─────────────┘│                        │   │
│  │  └───────────────────────────────────────┘                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      PLATFORM LAYER                                 │   │
│  │                                                                     │   │
│  │  GLFW (windowing)  |  OpenGL 3.3+ (GPU render)  |  CMake (build)  │   │
│  │  spdlog (logging)  |  simdjson (config/JSON)     |  vcpkg (deps)  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Module Breakdown

### 1. Presentation Layer — Charting & UI

**Tech:** Dear ImGui (docking branch) + ImPlot + custom OpenGL draw calls

This is the visual shell. Every panel is an ImGui window. Docking branch gives tear-off panels, tabbed charts, multi-monitor support — like ATAS workspaces.

#### 1.1 Chart Engine
The core chart renderer. Handles all standard chart types.

| Chart Type | How it renders | ATAS/Sierra equivalent |
|---|---|---|
| Candlestick | ImPlot custom series (OHLC rect + wicks) | Standard chart |
| Line / Area | `ImPlot::PlotLine()` | Overlay charts |
| Tick chart | 1 bar per N trades, variable time axis | Sierra tick chart |
| Range bars | 1 bar per N-point range | Sierra range |
| Renko | Fixed box size, no wicks | MT5 Renko |
| Volume bars | 1 bar per N contracts | Sierra volume chart |
| Heikin-Ashi | Smoothed OHLC formula | MT5 HA |

**Responsibilities:**
- Pan, zoom, scroll (mouse + keyboard)
- Time axis scaling (auto-fit, fixed)
- Price axis (linear, log, auto-scale)
- Crosshair with price/time readout
- Multi-timeframe linking (scroll one chart, all linked charts follow)

#### 1.2 Footprint Chart
The core differentiator vs basic charting tools. Per-cell bid/ask volume inside each candle.

```
     ┌──────────────┐
102.5│  150 × 320   │   ← ask volume × bid volume at this price level
102.4│   80 × 210   │
102.3│  420 × 110   │   ← imbalance: 420 asks vs 110 bids = selling pressure
102.2│   60 × 180   │
102.1│   30 ×  90   │
     └──────────────┘
```

**Data source:** Reconstructed from tick-by-tick T&S data, classified by aggressor side.

**Rendering:** Custom `ImDrawList` — colored rectangles per cell with text overlay. GPU-friendly since each cell is just a rect + 2 text draws.

**Variants:**
- Delta footprint (ask - bid per cell)
- Volume footprint (total per cell)
- Bid/Ask footprint (separate columns)
- Imbalance highlighting (diagonal stacking detection)

#### 1.3 Volume Profile
Horizontal histogram of volume at each price level.

**Types:**
- Session VP (reset each day)
- Visible Range VP (only what's on screen)
- Fixed Range VP (user-selected time range)
- Developing VP (builds in real-time)

**Key levels rendered:**
- POC (Point of Control — highest volume price)
- Value Area High / Low (70% of volume)
- HVN / LVN (High/Low Volume Nodes)

**Rendering:** `ImPlot::PlotBarsH()` for bars, custom annotations for POC/VA.

#### 1.4 DOM & Heatmap
Two related panels:

**DOM (Depth of Market):**
- Ladder-style display: price levels as rows, bid/ask size as colored bars
- Real-time order book from L2 data
- Size highlighting (large orders glow)
- One-click order entry from DOM

**DOM Heatmap (DeepCharts-style):**
- 2D grid: X = time, Y = price, Color = order book depth at that moment
- Shows where large orders were placed and pulled (iceberg detection)
- Ring buffer of L2 snapshots, rendered as `ImPlot::PlotHeatmap()`
- Configurable color gradient (depth → intensity)

#### 1.5 Tape / Time & Sales
Scrolling table of every trade.

| Time | Price | Size | Side | Flag |
|---|---|---|---|---|
| 09:30:01.234 | 102.45 | 500 | BUY | LARGE |
| 09:30:01.235 | 102.44 | 100 | SELL | |

**Features:**
- Color-coded rows (green = buy, red = sell)
- Size filtering (show only > N lots)
- Large block highlighting (adaptive threshold — existing TapeReader logic)
- Speed indicator (trades/second)
- Cumulative delta running total

#### 1.6 Drawing Tools
Overlay objects the user draws on charts.

- Trendlines, horizontal lines, rays
- Fibonacci retracement / extension
- Rectangles, ellipses
- Text annotations
- Anchored VWAP
- Measurement tool (price distance + % + bars)

**Storage:** Serialized per-chart in workspace JSON. Drawings persist across sessions.

#### 1.7 Indicator Overlay System
Indicators rendered on the price chart or in sub-panels.

**Built-in (C++):**
- SMA, EMA, VWAP, Bollinger Bands
- RSI, MACD, Stochastic
- ATR, ADX
- Cumulative Delta (as sub-chart)
- Volume (bar chart below price)

**Custom (Python):**
- User writes a Python function: `def calculate(bars) → series`
- Called via pybind11 bridge
- Result plotted as overlay or sub-panel

#### 1.8 Layout Manager
Manages the workspace — which panels are open, where they're docked, what symbols they display.

- ImGui docking branch handles panel arrangement
- Save/load workspace layouts as JSON
- Named workspaces ("Scalping", "Swing", "Backtest Review")
- Multi-monitor: panels can be dragged to separate OS windows
- Symbol linking groups: change symbol in one panel → linked panels follow

---

### 2. Application Layer

#### 2.1 Chart State Manager
Manages the non-visual state behind every chart.

**Responsibilities:**
- Owns bar data (OHLC arrays) for each chart
- Manages bar builders (tick → bars conversion)
- Tracks visible range, scroll position, zoom level
- Maintains indicator calculation state
- Handles timeframe switching (recalculate bars from ticks)

**Bar Builder Pipeline:**
```
Raw ticks (from MarketDataManager or HistoricalStore)
    │
    ├─ TimeBarBuilder    → 1m, 5m, 15m, 1h, 4h, D, W bars
    ├─ TickBarBuilder    → N-tick bars
    ├─ RangeBarBuilder   → N-point range bars
    ├─ VolumeBarBuilder  → N-contract volume bars
    └─ RenkoBuilder      → N-point renko bricks
```

Each builder is a stateful object. Feed it ticks, it emits completed bars.

#### 2.2 Backtest Controller
Runs strategies against historical data.

**Architecture:** Event-driven simulation. The backtest engine replays historical ticks through the same Strategy Engine (V1 analyzers) and a simulated order book.

```
Historical Data Store
        │
        ▼
  Replay Engine ──tick──▶ MarketDataManager (sim mode)
        │                        │
        │                  ┌─────┴──────┐
        │                  ▼            ▼
        │           StrategyEngine   SimBroker
        │           (6 analyzers)    (fills, slippage)
        │                  │            │
        │                  ▼            ▼
        │           Signal ────▶ Order ────▶ Position
        │                                      │
        ▼                                      ▼
  Performance Analytics              Trade Log / Journal
```

**Replay Engine:**
- Reads historical ticks from HDF5/binary store
- Feeds them through MarketDataManager at configurable speed
- Supports: real-time speed, max speed, step-by-step
- Clock is simulated — all components use `SimClock` instead of `system_clock`

**SimBroker (Simulated Broker):**
- Accepts orders from strategy
- Simulates fills with configurable:
  - Slippage model (fixed, random, volume-based)
  - Commission model (per-share, per-trade, tiered)
  - Latency model (fixed delay, random)
- Maintains position state, P&L

**Performance Analytics:**
- Equity curve
- Sharpe ratio, Sortino ratio, Calmar ratio
- Max drawdown (absolute + duration)
- Win rate, profit factor, average win/loss
- Monthly/daily returns heatmap
- Trade distribution analysis

**Optimization (Phase 2 of backtest):**
- Parameter sweep (grid search)
- Walk-forward analysis (in-sample → out-of-sample windows)
- Monte Carlo simulation on trade sequence
- Results displayed in Backtest Results panel

#### 2.3 Live Trading Controller
Manages real-money (or paper) order execution.

**Order Management System (OMS):**
```
Strategy Signal (BUY/SELL + score)
        │
        ▼
  Risk Manager ──── check limits ────▶ REJECT (if limits breached)
        │
        ▼ (approved)
  Order Router
        │
        ├──▶ IBKR Adapter → IBKRConnection → IB Gateway → Exchange
        │
        ▼
  Execution Monitor
        │
        ├── Fill confirmation → Position Manager
        ├── Partial fill → update working order state
        └── Rejection → alert + log
```

**Risk Manager (pre-trade):**
- Max position size per symbol
- Max total portfolio exposure
- Max loss per day (daily stop)
- Max loss per trade
- Max number of open positions
- Cooldown after N consecutive losses

**Position Manager:**
- Real-time P&L (unrealized from market data, realized from fills)
- Average entry price tracking
- Position aging (how long held)
- Auto-flatten at configurable time (e.g., 15:55 ET)

**Order Types:**
- Market, Limit, Stop, Stop-Limit
- Bracket orders (entry + profit target + stop loss)
- OCO (one-cancels-other)
- Trailing stop

#### 2.4 Strategy Engine (V1 — Preserved)
The existing 6-factor scoring pipeline carries forward unchanged:

```
MarketDataManager → StrategyEngine
                        ├── CPRCalculator
                        ├── CamarillaCalculator
                        ├── VPAAnalyzer
                        ├── BookFlipDetector
                        ├── AbsorptionDetector
                        ├── StackingAnalyzer
                        └── TapeReader
                              │
                        SignalAggregator → Score 0-6
```

**V2 additions:**
- Expose analyzer outputs to charting layer (draw CPR/Camarilla levels on chart)
- Expose signal events to backtest engine
- Python-scriptable strategy layer that can override or augment the 6-factor system

#### 2.5 Python Scripting Bridge
Embedded Python via **pybind11**. Allows users to write custom logic without recompiling C++.

**Use cases:**
- Custom indicators: `def rsi(bars, period=14) → np.array`
- Custom strategies: `def on_bar(bar, book, tape) → Signal`
- Backtest scripts: run parameter sweeps from Jupyter
- Data export: dump internal state to pandas DataFrames

**How it works:**
- C++ embeds Python interpreter (pybind11)
- Exposes C++ types to Python: `BarData`, `OrderBook`, `Trade`, `Signal`
- Python scripts live in `scripts/` directory
- Hot-reload: edit script → re-run without restarting app

---

### 3. Data Layer

#### 3.1 MarketDataManager (V1 — Preserved)
The existing observer pattern hub. No changes needed.

```
IMarketDataProvider (interface)
        │
IBKRAdapter (V1, existing)
        │
MarketDataManager (broadcasts to listeners)
        │
        ├── SymbolManager
        ├── StrategyEngine
        ├── ChartStateManager (NEW — feeds bar builders)
        └── HistoricalRecorder (NEW — writes ticks to disk)
```

#### 3.2 Historical Data Store
Persistent storage for tick and bar data. Required for backtesting and chart history.

**Format:** HDF5 (via HighFive C++ library) or memory-mapped binary files

**Structure:**
```
data/
├── ticks/
│   ├── NVDA/
│   │   ├── 2026-04-11.h5    (all ticks for the day)
│   │   └── 2026-04-10.h5
│   └── AAPL/
│       └── ...
├── bars/
│   ├── NVDA/
│   │   ├── 1m.h5            (pre-aggregated 1-minute bars)
│   │   ├── 5m.h5
│   │   └── 1d.h5
│   └── ...
└── snapshots/
    └── NVDA/
        └── 2026-04-11_book.h5   (L2 order book snapshots for heatmap replay)
```

**Data flow:**
- **Live recording:** `HistoricalRecorder` (new listener) writes every tick to disk
- **Backtest loading:** Replay Engine reads ticks from disk, feeds through pipeline
- **Chart scrollback:** When user scrolls past loaded data, fetch from disk or request from IBKR `reqHistoricalData`

#### 3.3 Bar Builder Pipeline
Converts raw ticks into bars of any type. Multiple builders can run simultaneously for the same symbol (one per chart timeframe).

**Interface:**
```cpp
class IBarBuilder {
public:
    virtual void onTick(const Tick& tick) = 0;
    virtual bool hasCompletedBar() const = 0;
    virtual Bar popCompletedBar() = 0;
    virtual Bar getCurrentBar() const = 0;  // developing bar
};
```

**Implementations:**
- `TimeBarBuilder` — closes bar on time boundary
- `TickBarBuilder` — closes bar after N ticks
- `RangeBarBuilder` — closes bar when H-L >= range
- `VolumeBarBuilder` — closes bar when cumulative volume >= threshold
- `RenkoBuilder` — closes brick when price moves >= box size

---

### 4. Platform Layer

#### 4.1 Windowing & Rendering
| Component | Library | Purpose |
|---|---|---|
| Window creation | GLFW 3.4+ | Cross-platform windows, input, OpenGL context |
| OpenGL loading | glad | Function pointer loading for OpenGL 3.3+ |
| ImGui | Dear ImGui (docking branch) | All UI panels, menus, dialogs |
| Plotting | ImPlot 0.17+ | Charts, heatmaps, bar charts |
| Font rendering | ImGui + TTF | Monospace for data, proportional for UI |

**Render loop:**
```cpp
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Application renders all panels
    app.render();   // chart panels, DOM, tape, backtest, etc.

    // Finish frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Handle multi-viewport (multi-monitor)
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    glfwSwapBuffers(window);
}
```

**Target:** 60 FPS with 5 chart panels open, streaming live data.

#### 4.2 Build System
CMake with vcpkg for dependency management.

```cmake
# V2 dependencies (via vcpkg)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(imgui REQUIRED)      # docking branch
find_package(implot REQUIRED)
find_package(spdlog REQUIRED)
find_package(HighFive REQUIRED)   # HDF5 wrapper
find_package(pybind11 REQUIRED)

# Platform-specific
if(WIN32)
    # Link ws2_32 for sockets (IBKR uses Winsock)
    target_link_libraries(dna_system ws2_32)
elseif(UNIX)
    target_link_libraries(dna_system pthread)
endif()
```

**vcpkg manifest (`vcpkg.json`):**
```json
{
    "name": "dna-trading-system",
    "version": "2.0.0",
    "dependencies": [
        "glfw3",
        "glad",
        "imgui[docking-experimental,glfw-binding,opengl3-binding]",
        "implot",
        "spdlog",
        "highfive",
        "pybind11",
        "simdjson"
    ]
}
```

#### 4.3 Cross-Platform Strategy

| Concern | Windows | Linux |
|---|---|---|
| Windowing | GLFW (same) | GLFW (same) |
| OpenGL | Desktop GL 3.3 | Desktop GL 3.3 |
| Sockets (IBKR) | Winsock (`ws2_32`) | POSIX sockets (`pthread`) |
| File paths | `\` (abstracted via `std::filesystem`) | `/` |
| Build | CMake + MSVC or MinGW | CMake + GCC/Clang |
| Package mgr | vcpkg | vcpkg |
| HDF5 | vcpkg builds it | vcpkg or system package |

**Rule:** All platform-specific code lives behind `#ifdef _WIN32` / `#ifdef __linux__` guards, contained in the platform layer. Nothing above the platform layer touches OS APIs directly.

---

## Directory Structure (V2)

```
DNA_Trading_System/
├── CMakeLists.txt
├── vcpkg.json                      # dependency manifest
├── config/
│   ├── symbols.json
│   ├── ibkr.json
│   └── workspaces/                 # saved layout files
│       └── default.json
├── include/
│   ├── core/                       # V1 preserved
│   │   ├── SymbolState.hpp
│   │   ├── SymbolManager.hpp
│   │   ├── IBKRConnection.hpp
│   │   ├── IMarketDataProvider.hpp
│   │   ├── IMarketDataListener.hpp
│   │   └── MarketDataManager.hpp
│   ├── adapters/                   # V1 preserved
│   │   └── IBKRAdapter.hpp
│   ├── strategy/                   # V1 preserved + extended
│   │   ├── StrategyEngine.hpp
│   │   ├── StrategyTypes.hpp
│   │   ├── SignalAggregator.hpp
│   │   └── analyzers/
│   │       ├── CPRCalculator.hpp
│   │       ├── CamarillaCalculator.hpp
│   │       ├── VPAAnalyser.hpp
│   │       ├── BookFlipDetector.hpp
│   │       ├── AbsorptionDetector.hpp
│   │       ├── StackingAnalyzer.hpp
│   │       └── TapeReader.hpp
│   ├── ui/                         # NEW — all rendering
│   │   ├── App.hpp                 # top-level app, owns render loop
│   │   ├── Theme.hpp               # colors, fonts, styling
│   │   ├── LayoutManager.hpp       # workspace save/load
│   │   ├── panels/
│   │   │   ├── ChartPanel.hpp      # candlestick / line / range / etc.
│   │   │   ├── FootprintPanel.hpp  # bid/ask volume cells
│   │   │   ├── VolumeProfilePanel.hpp
│   │   │   ├── DOMPanel.hpp        # depth of market ladder
│   │   │   ├── HeatmapPanel.hpp    # historical order book heatmap
│   │   │   ├── TapePanel.hpp       # time & sales
│   │   │   ├── OrderPanel.hpp      # order entry + management
│   │   │   ├── PositionPanel.hpp   # open positions + P&L
│   │   │   ├── BacktestPanel.hpp   # backtest config + results
│   │   │   └── IndicatorPanel.hpp  # sub-chart indicators
│   │   └── drawing/
│   │       ├── DrawingTool.hpp     # base class
│   │       ├── TrendLine.hpp
│   │       ├── FibRetracement.hpp
│   │       └── HorizontalLine.hpp
│   ├── charting/                   # NEW — chart data logic (non-visual)
│   │   ├── ChartStateManager.hpp
│   │   ├── BarBuilder.hpp          # IBarBuilder interface
│   │   ├── TimeBarBuilder.hpp
│   │   ├── TickBarBuilder.hpp
│   │   ├── RangeBarBuilder.hpp
│   │   ├── VolumeBarBuilder.hpp
│   │   └── RenkoBuilder.hpp
│   ├── backtest/                   # NEW
│   │   ├── BacktestController.hpp
│   │   ├── ReplayEngine.hpp
│   │   ├── SimBroker.hpp
│   │   ├── SimClock.hpp
│   │   ├── PerformanceAnalytics.hpp
│   │   └── Optimizer.hpp
│   ├── trading/                    # NEW
│   │   ├── LiveTradingController.hpp
│   │   ├── OrderRouter.hpp
│   │   ├── RiskManager.hpp
│   │   ├── PositionManager.hpp
│   │   └── OrderTypes.hpp
│   ├── data/                       # NEW
│   │   ├── HistoricalStore.hpp
│   │   ├── HistoricalRecorder.hpp
│   │   └── DataTypes.hpp
│   └── scripting/                  # NEW
│       └── PythonBridge.hpp
├── src/
│   ├── main.cpp                    # V2: init GLFW + ImGui, run render loop
│   ├── core/                       # V1 preserved
│   ├── adapters/                   # V1 preserved
│   ├── strategy/                   # V1 preserved
│   ├── ui/                         # NEW
│   │   ├── App.cpp
│   │   ├── Theme.cpp
│   │   ├── LayoutManager.cpp
│   │   ├── panels/
│   │   │   ├── ChartPanel.cpp
│   │   │   ├── FootprintPanel.cpp
│   │   │   ├── VolumeProfilePanel.cpp
│   │   │   ├── DOMPanel.cpp
│   │   │   ├── HeatmapPanel.cpp
│   │   │   ├── TapePanel.cpp
│   │   │   ├── OrderPanel.cpp
│   │   │   ├── PositionPanel.cpp
│   │   │   ├── BacktestPanel.cpp
│   │   │   └── IndicatorPanel.cpp
│   │   └── drawing/
│   │       └── ...
│   ├── charting/                   # NEW
│   │   ├── ChartStateManager.cpp
│   │   ├── TimeBarBuilder.cpp
│   │   └── ...
│   ├── backtest/                   # NEW
│   │   ├── BacktestController.cpp
│   │   ├── ReplayEngine.cpp
│   │   ├── SimBroker.cpp
│   │   └── PerformanceAnalytics.cpp
│   ├── trading/                    # NEW
│   │   ├── LiveTradingController.cpp
│   │   ├── OrderRouter.cpp
│   │   ├── RiskManager.cpp
│   │   └── PositionManager.cpp
│   ├── data/                       # NEW
│   │   ├── HistoricalStore.cpp
│   │   └── HistoricalRecorder.cpp
│   └── scripting/                  # NEW
│       └── PythonBridge.cpp
├── scripts/                        # Python user scripts
│   ├── indicators/
│   │   └── custom_rsi.py
│   └── strategies/
│       └── mean_reversion.py
├── external/                       # V1 preserved
│   ├── simdjson.h / .cpp
│   ├── ibkr/
│   └── libbid.a
├── data/                           # Historical data (gitignored)
│   ├── ticks/
│   ├── bars/
│   └── snapshots/
├── tests/
└── docs/
```

---

## Build Phases

### Phase 1: Window + Chart (you are here)
**Goal:** Get a window on screen with a candlestick chart rendering test data.

1. Set up vcpkg + CMake for Windows (MSVC or MinGW)
2. Integrate GLFW + glad + ImGui (docking) + ImPlot
3. Create `App` class with render loop
4. Render a basic candlestick chart with hardcoded OHLC data
5. Add pan/zoom/scroll
6. Wire MarketDataManager → ChartStateManager → TimeBarBuilder → chart

**Deliverable:** A window showing live candlestick chart from IBKR data.

### Phase 2: Order Flow Panels
**Goal:** Footprint, DOM, Volume Profile, Tape — the ATAS/DeepCharts features.

1. FootprintPanel — per-cell bid/ask from T&S data
2. DOMPanel — L2 order book ladder
3. HeatmapPanel — historical book depth visualization
4. VolumeProfilePanel — session VP with POC/VA
5. TapePanel — scrolling Time & Sales

**Deliverable:** Full order-flow analysis suite, streaming live data.

### Phase 3: Indicators + Drawing Tools
**Goal:** Built-in indicators, drawing tools, and Python scripting.

1. Indicator framework (SMA, EMA, RSI, MACD, BB, VWAP)
2. Drawing tools (trendlines, fibs, horizontals)
3. pybind11 integration for custom Python indicators
4. Workspace save/load

**Deliverable:** Feature parity with Sierra Chart for indicators + drawings.

### Phase 4: Backtesting Engine
**Goal:** Event-driven backtest with the 6-factor strategy.

1. Historical data store (HDF5)
2. HistoricalRecorder for live data capture
3. ReplayEngine for tick playback
4. SimBroker with slippage/commission models
5. PerformanceAnalytics (Sharpe, drawdown, equity curve)
6. BacktestPanel for results visualization

**Deliverable:** Run 6-factor strategy against historical data, see equity curve + stats.

### Phase 5: Live Trading
**Goal:** Real order execution through IBKR.

1. OrderRouter → IBKR order placement API
2. RiskManager (pre-trade checks)
3. PositionManager (real-time P&L)
4. OrderPanel (UI for order entry)
5. PositionPanel (open positions display)
6. Bracket orders, OCO, trailing stops

**Deliverable:** Place and manage orders from the DNA platform.

### Phase 6: Polish + Linux
**Goal:** Cross-platform build, performance, UX polish.

1. Linux build (GCC/Clang, POSIX sockets)
2. Multi-monitor support
3. Performance profiling (60 FPS target with 5 panels)
4. Config/theme customization
5. Error handling, logging (spdlog)

---

## Threading Model

```
┌──────────────────────┐
│    Main Thread        │  ← GLFW event loop + ImGui render (ALL UI here)
│    (render loop)      │     never blocked by data or computation
└──────────┬───────────┘
           │ reads from lock-free queues
           │
┌──────────▼───────────┐
│   Market Data Thread  │  ← IBKR EReader + processMessages
│   (V1, existing)      │     pushes ticks into MarketDataManager
└──────────────────────┘
           │
┌──────────▼───────────┐
│   Strategy Thread     │  ← Runs 6 analyzers per symbol per tick
│   (optional)          │     writes scores to SymbolManager (atomic)
└──────────────────────┘
           │
┌──────────▼───────────┐
│   Data Writer Thread  │  ← HistoricalRecorder writes ticks to HDF5
│   (background)        │     never blocks the data pipeline
└──────────────────────┘
           │
┌──────────▼───────────┐
│   Backtest Thread     │  ← ReplayEngine runs in background
│   (when backtesting)  │     signals main thread on completion
└──────────────────────┘
```

**Key constraint:** The main thread (render loop) must NEVER block. All data arrives via lock-free queues or double-buffered shared state. The UI reads the latest snapshot each frame.

---

## Key Design Decisions

### 1. Why ImGui over Qt?
- Qt is heavy (100MB+ runtime), ImGui is ~200KB
- ImGui redraws every frame — perfect for streaming market data that changes every frame anyway
- ImPlot gives us charts out of the box; Qt needs QCustomPlot or manual QPainter
- ImGui docking branch gives us ATAS-style workspace management for free
- No signals/slots abstraction overhead
- Easier to embed into existing C++ codebase

### 2. Why pybind11 over embedded Lua/V8?
- Simran already knows Python
- numpy/pandas available for backtesting analytics
- pybind11 is zero-overhead for type conversions
- Python ecosystem (TA-Lib, scikit-learn) available for strategy research
- Same language for scripting and Jupyter notebook analysis

### 3. Why HDF5 for historical data?
- Columnar storage — fast time-range queries
- Compression built in (LZ4/ZSTD)
- Memory-mapped reads — near-zero copy
- Well-supported on Windows and Linux
- Standard in quant finance (QuantConnect, Zipline use it)

### 4. Why vcpkg?
- Works on Windows and Linux identically
- CMake integration is native (`find_package()` just works)
- Has all our dependencies: GLFW, ImGui, ImPlot, spdlog, HDF5, pybind11
- Binary caching — build once, reuse

### 5. Why preserve V1 architecture?
- MarketDataManager, StrategyEngine, all 6 analyzers — they work
- V2 is additive: new listeners (chart, recorder), new UI layer on top
- No risky rewrites of battle-tested data pipeline
- Strategy engine is already listener-based — adding chart rendering is just another listener

---

## Performance Targets

| Metric | Target |
|---|---|
| Render FPS | 60 FPS with 5 panels open |
| Tick-to-screen latency | < 5ms (from data arrival to pixel on screen) |
| Bar build latency | < 1ms per tick per builder |
| Strategy score latency | < 1ms per symbol (V1 target, preserved) |
| Memory per symbol | < 5MB (ticks + bars + book snapshots) |
| Historical data read | > 1M ticks/second from HDF5 |
| Backtest speed | > 10x real-time for tick replay |
| Startup time | < 3 seconds to first chart |

---

## Architecture Review — Claude Sonnet 4.6

*Review date: 2026-04-12 — 10 suggestions based on codebase analysis and V2 architecture review.*

### 1. Bar Builder Pipeline — Missing Heikin-Ashi Builder

The document lists Heikin-Ashi as a chart type but `IBarBuilder` only has 5 implementations. Heikin-Ashi is not tick-driven — it's a **transform** applied to completed time bars (smoothed OHLC formula). Either add a `HeikinAshiBuilder` that wraps `TimeBarBuilder` output, or clarify in the doc that HA is a display mode / transform on time-based bars, not a standalone builder.

### 2. Threading Model — Lock-Free Queue Spec Is Vague

The doc says "lock-free queues or double-buffered shared state" but doesn't specify the implementation. Given the 60 FPS target with 5 panels, recommend:

- **`moodycamel::ConcurrentQueue`** or **`rigtorp::SPSCQueue`** — proven, header-only, 10-50x faster than `std::mutex` + `std::deque`
- **Double-buffer pattern** for DOM/heatmap data: main thread reads Buffer A while data thread writes Buffer B, then atomically swap pointers. Ideal for the DOM panel where you want the full book snapshot each frame without locking.

The current V1 code uses `std::mutex` in `SymbolManager` and `StrategyEngine`. For V2 with 5+ panels polling every frame, these will contend on the render path and cause stutters.

### 3. MarketDataManager Observer Pattern — No Unsubscribe Mechanism

`IMarketDataListener` has no `removeListener()` or unsubscribe on `MarketDataManager`. Once registered, a listener can't be removed. This breaks for:

- Closing a chart panel (zombie listeners keep receiving updates)
- Switching symbols (old symbol's listeners become orphans)
- Backtest replay (need to attach/detach replay engine dynamically)

**Suggestion:** Add `removeListener(IMarketDataListener*)` to `MarketDataManager`. Consider adding `virtual getSymbols()` to `IMarketDataListener` for filtered broadcasting — only send NVDA data to listeners who care about NVDA.

### 4. Historical Data Store — Consider Skipping HDF5 for Phase 1-2

HDF5 via HighFive is a heavy dependency (30+ MB library, build complexity, cross-platform quirks). For Phase 1-2 (chart + order flow panels), historical storage isn't needed yet.

**Suggestion:**

- **Phase 1-2:** Use **memory-mapped binary flat files** — one file per symbol per day, fixed struct per tick. `mmap` loads a day's ticks in microseconds, zero extra dependencies.
- **Phase 4 (backtest):** Migrate to HDF5 when you need columnar queries and compression.

The `HistoricalStore` interface abstracts this — swap the implementation later with zero app code changes.

### 5. SimClock — Define the Clock Interface Early

The doc mentions `SimClock` for backtesting but doesn't design it. This is critical because:

- Every time-dependent component (bar builders, strategy warm-up, session detection) needs it
- V1 code uses `std::chrono::system_clock` directly — backtest replay will use wall clock instead of simulated time
- Retrofitting later is painful; every `system_clock::now()` call must be found and replaced

**Suggestion:** Define a `Clock` interface now (even before Phase 4):

```cpp
class IClock {
public:
    virtual ~IClock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
};

class WallClock : public IClock { /* wraps system_clock */ };
class SimClock  : public IClock { /* returns simulated time, advances on replay */ };
```

Inject into `MarketDataManager`, `StrategyEngine`, and all `IBarBuilder` instances.

### 6. IBKR L2 Paper Account Limitation — Design Around It

Paper accounts cap at 3 simultaneous L2 depth subscriptions (IBKR error 309). The doc doesn't address this.

**Suggestion:** Add a **subscription budget manager**:

- Tracks active L2 depth subscriptions
- Prioritizes symbols (active chart = L2, background symbols = L1 only)
- Gracefully falls back to L1-only when budget is exhausted
- `IMarketDataProvider` should expose `getMaxDepthSubscriptions()` so the UI can explain why some symbols lack DOM data

### 7. AbsorptionDetector — Known Bug Should Be Documented

`AbsorptionDetector::detect()` accepts a `std::vector<TradeData>` parameter, but `StrategyEngine::onBookUpdate()` never provides trade data to it. The `recentTrades` vector is always empty — absorption detection relies solely on book-level wall tracking without cross-referencing trade volume.

This should be documented as a known gap. V2's `ChartStateManager` (which sees both book and trade updates) is the right place to fix it.

### 8. Python Scripting — Defer pybind11 to Phase 3+

pybind11 is a heavy dependency (build complexity, Python runtime embedding, ABI stability concerns). None of the V2 panels need Python yet.

**Suggestion:** Keep the `PythonBridge.hpp` interface sketch but don't add pybind11 to `vcpkg.json` or `CMakeLists.txt` until Phase 3. This keeps Phase 1-2 builds fast and dependency-light.

### 9. UI Panel Communication — Add an Event Bus

The doc describes panels but doesn't specify how they communicate. Examples:

- Clicking a tape row → highlight that price on the chart
- Clicking a DOM price → set order entry price
- Selecting a symbol → change all linked panels

**Suggestion:** Add an **event bus** / message broker:

```cpp
struct SymbolChanged { std::string symbol; };
struct PriceClicked  { std::string symbol; double price; };

class EventBus {
    // template<EventType> subscribe(callback)
    // template<EventType> publish(event)
};
```

This avoids tight coupling between panels. ImGui docking handles panel lifecycle — the event bus handles inter-panel communication.

### 10. Error Handling & Reconnection — Missing Section

The V2 doc has no section on error handling. The V1 `IBKRConnection` handles disconnects but the architecture doesn't specify:

- **Auto-reconnect** policy for IB Gateway (exponential backoff? max retries?)
- **Data staleness detection** — if no ticks arrive for N seconds, mark symbol STALE (`DataStatus::STALE` enum exists but is never set)
- **UI notification** — panels should show "NO DATA" / "STALE" / "RECONNECTING" states
- **Graceful shutdown** — when IB Gateway disconnects, stop strategy engine, freeze UI data, don't crash

**Suggestion:** Add a "Resilience" section to the architecture covering reconnect, staleness detection, and UI error states.

---

### Priority Summary

| Priority | Suggestion | Reason |
|---|---|---|
| **High** | Add `IClock` interface now | Hard to retrofit, blocks backtesting |
| **High** | Add `removeListener()` + filtered broadcast | Panels need to detach, symbol filtering |
| **High** | Design panel event bus | Panels must communicate without coupling |
| **Medium** | Defer HDF5, start with mmap binary | Reduce Phase 1-2 dependency burden |
| **Medium** | Add resilience/reconnect section | Prevents crashes on disconnect |
| **Medium** | Dual build targets (GUI + console) | Enables testing without GUI |
| **Low** | Subscription budget manager for L2 | Known limitation, plan ahead |
| **Low** | Lock-free queue specifics | Profile first, optimize later |
| **Low** | Defer pybind11 | No Phase 1-2 need |