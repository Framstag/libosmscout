# Proposal

## Why

The map library carries two tile cache implementations. One of them is used by everything; the other
is dead: it is declared, defined, compiled, exported and installed as public API, and not a single
caller exists in the tree — not in the libraries, the renderers, the clients, the bindings, the demos
or the tests. The dead one shares the naming domain of the live one, so every future change in this
area (the memory budget of the map data caches is the next one) has to argue which of the two it
means. Alongside it, a few more declarations in the same area have no caller either, and two
accessors that the memory budget work does need should be kept for it.

## What Changes

- **BREAKING**: Remove the unused tile cache template, its tile class and its reference alias,
  including the public header and its implementation file, from both build systems and from the
  installed header set. External code that includes that header stops compiling (no in-tree code does).
- Remove the unused tile cache reference alias of the used tile cache.
- Remove the unused cache cleanup method of the map service.
- Keep the two size accessors of the map service, which the memory budget change needs for reporting,
  and state that explicitly so the two changes do not overlap.
- No behaviour change: the remaining implementation is the one that is used today.

## Capabilities

### New Capabilities

<!-- None: removing unused declarations changes no behaviour, so no spec is created. -->

### Modified Capabilities

<!-- None: no requirement is affected. -->

This change therefore opts out of specs in its change metadata, because no spec-level behaviour
changes (pure removal of unused code).

## Impact

- `libosmscout-map/include/osmscoutmap/MapTileCache.h` — deleted (public, installed header).
- `libosmscout-map/src/osmscoutmap/MapTileCache.cpp` — deleted.
- `libosmscout-map/CMakeLists.txt` and `libosmscout-map/meson.build` — the removed files are dropped
  from the source and header lists.
- `libosmscout-map/include/osmscoutmap/DataTileCache.h` — the unused tile cache reference alias is
  removed.
- `libosmscout-map/include/osmscoutmap/MapService.h` and
  `libosmscout-map/src/osmscoutmap/MapService.cpp` — the unused cache cleanup method is removed; the
  size accessors stay.
- Downstream packaging and bindings: no binding wraps the removed types, so no binding source changes;
  the installed header set shrinks by one header.
- Documentation: a note where the tile cache is documented, so the remaining cache is unambiguous.
