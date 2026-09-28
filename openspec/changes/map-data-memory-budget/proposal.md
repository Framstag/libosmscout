# Proposal

## Why

A client that opens several databases — a basemap plus the region databases of the areas a user
travels through — holds a map data cache for every one of them. The size of each of those caches is
fixed and independent of what the user is currently looking at, and the caches are never shrunk while
the application keeps rendering. Resident memory therefore grows with the number of open databases
even though only a few of them supply the data of the visible map, and on mobile that is the term
that can push the process over its memory limit.

## What Changes

- Introduce a **total memory budget** for map data cache contents that is shared by all open
  databases instead of a per-database size that applies to each of them independently.
- Account cached content in **comparable units**, so that a database holding large polygon-heavy
  objects weighs more than one holding the same number of small objects, and so that a budget can be
  expressed and reported as a memory figure.
- Distribute the budget by **relevance to the view**: databases whose geographic extent intersects
  the current view share the budget; databases outside the view keep only a floor.
- **Bound and release**: a database that leaves the view is reduced to its floor, and a database that
  stays outside the view for a longer idle period has its caches released entirely, so idleness
  actually returns memory rather than only capping it.
- Make the **budget configurable and observable**: a client can set the total budget and read the
  current usage; the budget is off at library level so that existing tools, demos and measurements
  keep their current behaviour, and the shipped clients impose a mobile default.
- Keep the existing per-database cache size setting meaningful for a single database when no budget
  is configured.
- Record two pre-existing defects found while exploring this area, neither introduced here: the idle
  release path of the client has no caller at all, and the "last usage" time of a database is
  refreshed by every render even when that database contributed no data to it.

## Capabilities

### New Capabilities

- `map-data-memory-budget`: a bounded, viewport-aware and reportable memory budget for the map data
  caches of all open databases, including accounting in comparable units, distribution by relevance
  to the view with hysteresis, idle release, and configuration and reporting.

### Modified Capabilities

<!-- None: no existing spec covers the map data caches of the databases or their memory. -->

## Impact

- `libosmscout-map/` — the map service that owns a database's tile cache (budget participation,
  relevance of a database to a view, idle release, reporting), the tile cache itself (accounting and
  resizing), and the new budget/accounting type. Public headers in
  `libosmscout-map/include/osmscoutmap/` gain the budget type and the report API.
- `libosmscout/` — the database and its data files (runtime resize of the object caches, keeping the
  parameter of a not yet created data file in sync) and the generic cache used by the data files
  (size accounting and resizing). Public headers in `libosmscout/include/osmscout/db/` and
  `libosmscout/include/osmscout/io/` are affected.
- `libosmscout-client/` — the thread that owns the open databases (owner of the shared budget, idle
  detection, the currently unused release call).
- `libosmscout-client-qt/` — the client builder (budget configuration and default) and the render and
  load jobs (activity reporting), `OSMScoutQt`, `MapWidget`.
- `libosmscout-client-java/` — the JNI bridge (budget configuration and default, the per-database
  cache size application on every render, the Java API and its Javadoc).
- `Tests/` — new unit tests for accounting, distribution, hysteresis and release; existing cache and
  conversion tests must keep passing unchanged.
- `Demos/` and `Tests/src/PerformanceTest.cpp` — unchanged behaviour (budget off by default), their
  cache size arguments stay meaningful.
- `TODO.md` — the two pre-existing defects above, for processing in other changes.
- `Documentation/` — a note on the budget, its units and its defaults, where the client behaviour is
  documented.
