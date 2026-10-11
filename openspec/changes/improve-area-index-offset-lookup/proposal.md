# Proposal

## Why

A lookup on the area index does work for every area type the index holds, even when the caller asks
for a handful of types. Its cost therefore follows the size of the type set rather than the size of
the request, so it grows with every type added to the type definition. That is the one lookup in the
database with this property: the node index already answers a request at the cost of the request.
Today the effect is small (444 indexed area types); the parked type set would take it to 1097, and
the unparking work is waiting on exactly this kind of measurement.

## What Changes

- An area-index lookup examines the types its request names, not the types the index happens to
  hold. The number of types the lookup touches follows the caller's request.
- The result is unchanged for every request: the same offsets (the current lookup collects them in a
  deduplicating container, so their order within the returned vector is not part of the contract) and
  the same set of types reported as loaded.
- The property is stated as a requirement with a scenario the test suite can assert, so a later
  growth of the type set cannot reintroduce the dependency without a failing test.
- The area index's sibling lookup becomes the reference the requirement is written against, so the
  two indexes are held to the same rule instead of one of them being the exception.

## Capabilities

### New Capabilities

- `area-index-lookup`: what a lookup on the area index is allowed to cost relative to the request it
  serves, and the guarantee that its result — the offsets it returns and the types it reports as
  loaded — does not change.

### Modified Capabilities

None. No existing capability's requirements change. The offsets and loaded types a caller receives,
the rendering and search paths that consume them, and the database files on disk all stay as they
are; only the cost of producing the same answer changes.

## Impact

- `libosmscout/src/osmscout/db/AreaIndex.cpp` — the lookup path (`GetOffsets` and the per-type helper
  it calls).
- `libosmscout/src/osmscout/db/AreaNodeIndex.cpp` — read only. Its lookup is the reference behaviour
  the requirement is written against and is not changed by this proposal.
- `libosmscout/include/osmscout/db/AreaIndex.h` — only if the internal per-type access changes; the
  public signature stays.
- `Tests/src/ThreadedDatabaseTest.cpp`, `Tests/CMakeLists.txt`, `Tests/meson.build` — the area index
  is covered only through the integration path today (no `Tests/src/*Area*` unit test exists). The
  new requirement needs a case that asserts both the unchanged result and the request-bound cost,
  registered in both build systems.
- No database format, type definition, style sheet, generated file or public API contract is
  affected.
- The absolute gain is small at the current type set and grows with the parked one, so this is
  insurance for the unparking work rather than a measurable win today. It is taken now because it is
  cheap and because the unparking gate waits on the index-side items.
