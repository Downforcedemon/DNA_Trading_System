# Phase 2: Provider-Agnostic Architecture Implementation

## Overview
Refactor the DNA Trading System to support multiple market data providers (Databento, IBKR, dxFeed, etc.) through a common abstraction layer with runtime provider switching.

---

## Architecture Goals

1. **Provider Independence** - Switch between data providers without code changes
2. **Runtime Switching** - Change providers on-the-fly via command
3. **Observer Pattern** - Multiple components can listen to market data
4. **Data Source Tracking** - Know which provider each data point came from
5. **Minimal Disruption** - Keep existing IBKR code working during transition

---

## Implementation Order

### Step 1: Create Core Interfaces
**Files:** `IMarketDataProvider.hpp`, `IMarketDataListener.hpp`

Pure abstract interfaces that define contracts for providers and data consumers.

---

### Step 2: Update SymbolState Data Structure
**File:** `include/core/SymbolState.hpp`

Add fields to track data source and timestamp of updates.

---

### Step 3: Create MarketDataManager
**Files:** `MarketDataManager.hpp`, `MarketDataManager.cpp`

Central hub that owns the active provider and notifies all listeners using Observer pattern.

---

### Step 4: Create IBKRAdapter
**Files:** `IBKRAdapter.hpp`, `IBKRAdapter.cpp`

Wraps existing IBKRConnection to implement the common interface. Uses Adapter pattern to avoid modifying working code.

---

### Step 5: Update SymbolManager
**Files:** `SymbolManager.hpp`, `SymbolManager.cpp`

Implement IMarketDataListener interface. Remove direct coupling to IBKRConnection.

---

### Step 6: Create DatabentoAdapter
**Files:** `DatabentoAdapter.hpp`, `DatabentoAdapter.cpp`

New adapter for Databento integration with Level 2 order book support for 20+ symbols.

---

### Step 7: Update main.cpp
**File:** `main.cpp`

Replace direct IBKRConnection usage with MarketDataManager. Load provider from config file.

---

### Step 8: Add Runtime Switching
**Implementation:** Command handler and provider switching logic

Enable switching between providers at runtime with automatic re-subscription.

---

### Step 9: Add Config File Support
**File:** `config/symbols.json`

Extend config to specify active provider and credentials for each provider.

---

## File Structure After Phase 2

```
DNA_Trading_System/
├── include/
│   ├── core/
│   │   ├── IMarketDataProvider.hpp       [NEW]
│   │   ├── IMarketDataListener.hpp       [NEW]
│   │   ├── MarketDataManager.hpp         [NEW]
│   │   ├── SymbolState.hpp               [UPDATED]
│   │   ├── SymbolManager.hpp             [UPDATED]
│   │   └── IBKRConnection.hpp            [UNCHANGED]
│   │
│   └── adapters/
│       ├── IBKRAdapter.hpp               [NEW]
│       └── DatabentoAdapter.hpp          [NEW]
│
├── src/
│   ├── core/
│   │   ├── MarketDataManager.cpp         [NEW]
│   │   ├── SymbolManager.cpp             [UPDATED]
│   │   └── IBKRConnection.cpp            [UNCHANGED]
│   │
│   ├── adapters/
│   │   ├── IBKRAdapter.cpp               [NEW]
│   │   └── DatabentoAdapter.cpp          [NEW]
│   │
│   └── main.cpp                          [UPDATED]
│
└── docs/
    ├── PHASE_2_PROVIDER_ABSTRACTION.md   [THIS FILE]
    └── DESIGN_DECISIONS.md               [COMPANION DOC]
```

---

## Data Flow Architecture

**Before Phase 2:**
```
IBKR API → IBKRConnection → SymbolManager
```

**After Phase 2:**
```
Provider API → Adapter → MarketDataManager → Listeners
                                             ├─ SymbolManager
                                             ├─ MarketDNAStrategy (future)
                                             └─ Logger (future)
```

---

## Testing Strategy

### Test 1: IBKRAdapter Works Like Before
Verify existing IBKR functionality unchanged through new abstraction layer.

### Test 2: DatabentoAdapter Integration
Connect to Databento and verify Level 2 order book data for 20+ symbols.

### Test 3: Runtime Switching
Test switching between providers on-the-fly with automatic re-subscription.

---

## Success Criteria

✅ Can switch between IBKR and Databento at runtime
✅ SymbolManager tracks data source per symbol
✅ All existing IBKR functionality still works
✅ Can monitor 20+ symbols with Level 2 data via Databento
✅ Observer pattern allows multiple listeners
✅ No code changes needed to add new providers

---

## Future Enhancements (Post Phase 2)

1. **Dual Provider Mode** - Run both providers simultaneously
2. **Provider Health Monitoring** - Track latency, gaps, errors
3. **Automatic Failover** - Switch to backup if primary fails
4. **Historical Data Integration** - Backfill from Databento
5. **MarketDNAStrategy Component** - Separate listener for order flow analysis
