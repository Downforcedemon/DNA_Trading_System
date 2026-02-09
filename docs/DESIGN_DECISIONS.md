# Provider-Agnostic Architecture - Design Decisions

## Document Purpose
This document explains the "why" behind our architectural choices for the provider abstraction layer in the DNA Trading System.

---

## Core Design Questions and Answers

### Q1: How Generic Should the Interface Be?

**Decision:** Trading-specific interface (not overly generic)

**Why:**
- Market data has well-defined concepts: prices, order books, trades
- Too generic = loss of type safety, harder to use
- Too specific = locked into one provider's model
- Sweet spot: Common trading concepts that all providers understand

**Example:**
- ✅ Good: `onOrderBookUpdate(symbol, bids[], asks[], timestamp)`
- ❌ Too generic: `onDataUpdate(symbol, dataType, void* data)`
- ❌ Too specific: `onDatabentoMBOUpdate(databento_mbo_msg* msg)`

**Trade-off:**
We chose clarity and type safety over maximum flexibility. If a future provider has unique features, we can extend the interface.

---

### Q2: Pure Interface vs. Base Class with Shared Logic?

**Decision:** Pure abstract interface (for now)

**Why:**
- Start simple, add complexity only when needed
- Different providers have very different implementations
- No obvious shared logic yet (IBKR uses callbacks, Databento uses different model)
- Easier to understand and maintain
- Can refactor to base class later if we see duplication

**Future Consideration:**
If we notice IBKRAdapter and DatabentoAdapter duplicating subscription tracking logic, we might introduce a base class then.

---

### Q3: How Should Data Flow to Your System?

**Decision:** Observer Pattern with callbacks

**Why:**
- Multiple components need the same market data (SymbolManager, Strategy, Logger)
- Decouples data producers from consumers
- Easy to add new listeners without modifying existing code
- Standard pattern for event-driven systems like trading platforms

**Alternative Considered:**
Direct calls from adapter to SymbolManager - Rejected because it couples adapter to specific components and prevents multiple listeners.

**Real-World Analogy:**
Like subscribing to a YouTube channel. The channel (MarketDataManager) notifies all subscribers (listeners) when new content (market data) arrives.

---

### Q4: Should SymbolManager Know About Providers?

**Decision:** Yes - SymbolManager is provider-aware

**Why:**
- Enables data quality comparison across providers
- Useful for debugging (which provider gave this price?)
- Allows tracking staleness (timestamp of last update)
- Supports future multi-provider scenarios (compare IBKR vs Databento prices)

**What This Means:**
SymbolState now includes:
- `dataSource` field - "databento", "ibkr", etc.
- `lastUpdate` timestamp - when this data arrived

**Alternative Considered:**
Provider-agnostic SymbolManager - Rejected because we lose valuable metadata about data origin and freshness.

---

### Q5: How Should Provider Switching Work?

**Decision:** Runtime switching with single active provider

**Why:**
- Flexibility to switch without restarting system
- Useful for testing different providers live
- Can react to provider outages
- Single active provider = simpler, less resource usage

**How It Works:**
1. User command: `switch databento`
2. Disconnect current provider
3. Connect new provider
4. Re-subscribe to all symbols
5. Resume data flow

**Trade-off:**
Brief data gap (milliseconds) during switch vs. complexity of dual-provider mode.

**Alternative Considered:**
Dual-provider mode (both connected) - Deferred to future enhancement. More complex, higher bandwidth usage, but enables comparison.

---

### Q6: Should We Build Full Abstraction from Day 1?

**Decision:** Yes - provider-agnostic architecture from the start

**Why:**
- Primary use case is Databento for data, IBKR for execution
- Already know we need multiple providers
- Harder to refactor later when code is entangled
- Clean separation of concerns from the beginning
- Future-proof for adding dxFeed, Alpaca, etc.

**What We're NOT Doing:**
- Not optimizing prematurely for Databento-specific features
- Not building dual-provider mode yet (can add later)
- Not implementing all possible data types (just what we need)

**Alternative Considered:**
Build Databento directly into core, abstract later - Rejected because refactoring working code is risky and time-consuming.

---

## Architectural Patterns Used

### 1. Strategy Pattern
**What:** Define family of algorithms (providers), make them interchangeable
**Where:** IMarketDataProvider interface with multiple implementations
**Why:** Enables runtime provider selection without changing client code

### 2. Adapter Pattern
**What:** Convert interface of a class into another interface clients expect
**Where:** IBKRAdapter wraps IBKRConnection
**Why:** Don't modify working IBKR code; translate to common interface

### 3. Observer Pattern
**What:** Define one-to-many dependency; when one object changes, dependents notified
**Where:** MarketDataManager notifies all IMarketDataListener instances
**Why:** Decouple data source from consumers; enable multiple listeners

### 4. Factory Pattern (Future)
**What:** Create objects without specifying exact class
**Where:** `createProvider("databento")` returns appropriate adapter
**Why:** Centralize provider instantiation logic

---

## Layered Architecture Rationale

### Layer 1: Raw Market Data (Provider-Agnostic)
**Purpose:** What providers give you (prices, order books, trades)
**Why Separate:** Different providers supply similar data in different formats

### Layer 2: Strategy Processing (Market DNA)
**Purpose:** What your strategy needs (order flow analysis, signals)
**Why Separate:** Strategy logic should be independent of data source

**Benefit of Separation:**
- Test strategy with historical data without live connection
- Swap data providers without changing strategy
- Reuse strategy with different data sources

---

## Key Design Principles Applied

### 1. Separation of Concerns
- Data acquisition (adapters) separate from data storage (SymbolManager) separate from strategy (MarketDNA)
- Each component has single, well-defined responsibility

### 2. Open/Closed Principle
- System is open for extension (add new providers via new adapters)
- System is closed for modification (don't change existing working code)

### 3. Dependency Inversion
- High-level code (SymbolManager) depends on abstraction (IMarketDataListener)
- Low-level code (adapters) depends on abstraction (IMarketDataProvider)
- Neither depends on concrete implementations

### 4. YAGNI (You Aren't Gonna Need It)
- Not building dual-provider mode yet
- Not implementing every possible data type
- Not creating base class until we see duplication
- Build what's needed now, add complexity only when required

---

## Performance Considerations

### Why Abstraction Doesn't Hurt Performance

**Concern:** Virtual function calls add overhead

**Reality:**
- Virtual function call: ~1-2 nanoseconds
- Network latency: 1-50 milliseconds (1,000,000+ times slower)
- Bottleneck is network, not code

**Measurement:**
```
Network to exchange: 1,000,000 nanoseconds (1ms)
Virtual function:            2 nanoseconds
Overhead:                    0.0002%
```

**Conclusion:** Abstraction cost is negligible compared to I/O.

---

## Risk Mitigation

### Risk 1: Provider Has Unique Feature Not in Interface

**Mitigation:**
- Extend interface when needed
- Provider-specific extensions in adapter
- Don't over-generalize upfront

### Risk 2: Breaking Changes to Working IBKR Code

**Mitigation:**
- Use Adapter pattern (wrap, don't modify)
- Keep IBKRConnection unchanged
- Extensive testing before switching to new architecture

### Risk 3: Performance Degradation

**Mitigation:**
- Observer pattern with direct function calls (not message queue)
- Measure before/after latency
- Profile if issues arise

---

## Alternative Architectures Considered and Rejected

### Alternative 1: Direct Integration (No Abstraction)
**What:** Build Databento directly into SymbolManager

**Pros:** Faster to implement initially
**Cons:** Hard to switch providers later, tight coupling, not extensible
**Verdict:** ❌ Rejected - shortsighted for multi-provider system

### Alternative 2: Message Queue Between Layers
**What:** Use queue (e.g., ZeroMQ) between adapter and consumers

**Pros:** Complete decoupling, async processing
**Cons:** Added complexity, latency, harder to debug
**Verdict:** ❌ Rejected - overkill for current needs, can add later if needed

### Alternative 3: Plugin System with Dynamic Loading
**What:** Load provider adapters as .so/.dll files at runtime

**Pros:** Ultimate flexibility, hot-swappable providers
**Cons:** Complex build system, harder debugging, platform-specific
**Verdict:** ❌ Rejected - over-engineering for 2-3 providers

---

## Design Evolution Path

### Phase 1 (Current): Single Provider, Direct Integration
- IBKRConnection directly updates SymbolManager
- Works, but not extensible

### Phase 2 (This Refactor): Provider-Agnostic with Single Active Provider
- Interface-based design
- Runtime provider switching
- Observer pattern for multiple listeners

### Phase 3 (Future): Multi-Provider with Data Fusion
- Multiple providers active simultaneously
- Compare data quality
- Intelligent routing (best price from multiple sources)

### Phase 4 (Advanced): Historical Data Integration
- Backfill from Databento
- Strategy backtesting with real tick data
- Replay mode for testing

---

## Lessons from Market DNA Strategy Requirements

### What Market DNA Strategy Needs:
1. **Level 2 Order Book** - Full depth, all price levels
2. **Order Flow at Camarilla Levels** - S3/R3/S4/R4 analysis
3. **Tape Reading** - Time & Sales, absorption detection
4. **20+ Symbols** - Multi-symbol monitoring

### How Architecture Supports This:
1. **IMarketDataProvider.subscribeLevel2()** - Explicit Level 2 support
2. **onOrderBookUpdate()** - Delivers full order book to strategy
3. **onTradeUpdate()** - Time & Sales events
4. **No symbol limits** - Provider-dependent, Databento handles 20+

### Why This Matters:
Architecture is designed around strategy needs, not provider capabilities. Strategy defines requirements, providers fulfill them.

---

## Conclusion

This architecture balances:
- **Flexibility** - Easy to add providers
- **Simplicity** - Not over-engineered
- **Performance** - Minimal overhead
- **Maintainability** - Clear separation of concerns
- **Practicality** - Solves real problem (Databento + IBKR)

We chose provider-agnostic design from day 1 because:
1. We know we need multiple providers
2. Refactoring later is harder
3. Cost is low, benefit is high
4. Aligns with professional software practices
