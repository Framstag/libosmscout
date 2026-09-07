## Why

The type definition (`map.ost`) has grown from 671 to 1527 types. Type resolution during import (`GetNodeType`, `GetWayAreaType`, `GetRelationType`) scans every type and evaluates its tag conditions for every imported object. Measured impact: +47% Preprocess time (16 threads, PBF input), with ways the worst affected (500k ways: 2.12s -> 4.41s). The cost grows linearly with type count and will keep growing as more types are added.

## What Changes

- Add a fast path for type resolution during import that avoids evaluating tag conditions for types that cannot match the object's tags.
- Preserve the exact resolution semantics: first matching type in type-definition order wins; way and area types resolved in the same pass.
- Add a benchmark test that measures type resolution cost per call, so regressions from type growth are visible.
- No change to the type definition format, the database format, or the public type resolution API.

## Capabilities

### New Capabilities

- `import-type-resolution`: Fast and correct resolution of OSM object types from tags during import, independent of the number of defined types.

### Modified Capabilities

- None.

## Impact

- `libosmscout/src/osmscout/TypeConfig.cpp` — type resolution methods (`GetNodeType`, `GetWayAreaType`, `GetRelationType`), type registration.
- `libosmscout/include/osmscout/TypeConfig.h` — internal index structures.
- `libosmscout/include/osmscout/Tag.h` — condition tree introspection (tag key extraction).
- `libosmscout/src/osmscout/ost/Parser.cpp` — condition construction (if key extraction happens at parse time).
- `Tests/src/TypeResolutionPerformanceTest.cpp` — new benchmark (already added).
- `Tests/CMakeLists.txt` — benchmark registration (already added).
- Import pipeline (`libosmscout-import/`) — no API change, benefits automatically.
