# Proposal

## Why

`libosmscout-client-java/src/search_scope.h` states in its own header comment that it is kept free of
libosmscout includes "so it can be unit-tested on the host without database/file I/O". No such test
exists: no test target references the file in either build system, and nothing under `Tests/` includes
it. Its two decisions — how a hierarchy depth maps onto the OSM admin_level scale, and from which
parent level a search may widen its scope — therefore rest on reading the code, although both are pure
functions that need no database to check.

## What Changes

- The two decisions get host unit tests: the depth-to-level mapping, the bounded scope expansion and
  the two together, so the level cap can be read against a real hierarchy chain.
- The tests pin the boundaries that carry meaning: the root, the cap itself, a level coarser than the
  cap, an unknown parent level, and the cap value the search was tuned for.
- Only a test target is added. The helper's behavior, its header, the search that uses it and both
  build systems' production targets are unchanged.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

None. The tests document and pin behavior that already exists and is already described by the header;
no requirement and no observable behavior changes, so the change carries no spec delta and its
`.openspec.yaml` sets `skip_specs: true`.

## Impact

- `Tests/src/SearchScopeTest.cpp` — new; the only source file added.
- `Tests/CMakeLists.txt` and `Tests/meson.build` — the new test target is registered in both, with the
  include path of `libosmscout-client-java/src` so the test can include the header the way its own
  users do.
- `TODO.md` — records two pre-existing issues this change surfaced: the helper's location outside the
  include path of any test, and the absent check that the two build systems register the same tests.
- No production code, no public API, no database file format, no dependency.
