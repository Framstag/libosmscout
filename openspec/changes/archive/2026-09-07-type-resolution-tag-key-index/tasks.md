## 1. Condition key extraction

- [x] 1.1 Add a `CollectPrimaryKeys` virtual method to `TagCondition` (`libosmscout/include/osmscout/Tag.h`) and implement it in all five subclasses (`TagBoolCondition` AND/OR, `TagNotCondition`, `TagExistsCondition`, `TagBinaryCondition`, `TagIsInCondition`); verify the project builds (spec: import-type-resolution/Correct type resolution)
- [x] 1.2 Implement leaf extraction (single tag -> key) for `TagExistsCondition`, `TagBinaryCondition`, `TagIsInCondition`; verify with unit tests that each leaf reports its referenced tag (spec: import-type-resolution/Resolution cost independent of type count)
- [x] 1.3 Implement composite extraction: AND -> first child key, OR -> each branch key, NOT -> inner key; verify with unit tests covering nested AND/OR, multi-key OR (`"waterway"=="riverbank" OR ("natural"=="water" AND "water"=="river")`), and NOT-in-AND combinations (spec: import-type-resolution/Correct type resolution)

## 2. Index construction

- [x] 2.1 Add index members and a `TypeConditionEntry` struct (type, condition, type index) to `TypeConfig` (`libosmscout/include/osmscout/TypeConfig.h`): per-geometry-kind maps from `TagId` to entries plus a fallback list per kind; verify the project compiles (spec: import-type-resolution/Resolution cost independent of type count)
- [x] 2.2 Build the index in `TypeConfig::RegisterType` by walking each type's condition tree with `CollectPrimaryKeys`, adding conditions to the keyed lists and NOT-containing conditions to the fallback lists; verify with a unit test that loading the real `map.ost` yields the 39 primary keys, every condition indexed or fallen back, and no type skipped (spec: import-type-resolution/Correct type resolution)
- [x] 2.3 Add a unit test that builds a synthetic `TypeConfig` with way+area, node, and relation types and asserts the index contains the expected key-to-condition mappings (spec: import-type-resolution/Correct type resolution)

## 3. Dispatch implementation

- [x] 3.1 Implement the dispatch path in `TypeConfig::GetNodeType`: iterate the object's tags, collect candidate entries, dedupe, sort by type index, evaluate in order, then evaluate the fallback list; verify with a unit test that dispatch results equal the linear scan results for representative tag sets (spec: import-type-resolution/Resolution order preserved, No-match resolution)
- [x] 3.2 Implement the dispatch path in `TypeConfig::GetWayAreaType`: one pass over the candidate set, setting wayType, areaType, or both per the condition's geometry mask, with the fallback list evaluated after; verify with unit tests for way-only, area-only, and both-capable matches (spec: import-type-resolution/Correct type resolution, Resolution order preserved)
- [x] 3.3 Implement the dispatch path in `TypeConfig::GetRelationType`; verify with unit tests for multipolygon, route, and restriction relations (spec: import-type-resolution/Correct type resolution)
- [x] 3.4 Add a fuzz correctness test: generate random tag sets from registered tag names and assert dispatch results equal the linear scan results across `GetNodeType`, `GetWayAreaType`, and `GetRelationType` (spec: import-type-resolution/Resolution order preserved, Backward compatibility)

## 4. Benchmark

- [x] 4.1 Verify `Tests/src/TypeResolutionPerformanceTest.cpp` builds and runs against both the 671-type and 1527-type `map.ost`, and record the baseline per-call costs (spec: import-type-resolution/Resolution performance benchmark)
- [x] 4.2 Extend the benchmark (if needed) so it always includes no-match and late-match tag sets for all three resolvers, and verify the output reports per-call cost for each set (spec: import-type-resolution/Resolution performance benchmark)
- [x] 4.3 After the dispatch implementation, re-run the benchmark and verify no-match and late-match per-call costs no longer scale linearly with type count (target: proportional to tags on the object, not type total) (spec: import-type-resolution/Resolution cost independent of type count)

## 5. Integration verification

- [x] 5.1 Build the full project without errors (CMake and Meson configuration compile cleanly) (spec: import-type-resolution/Backward compatibility)
- [x] 5.2 Run the existing test suite (`ctest` on the CMake build) and confirm no regressions (spec: import-type-resolution/Backward compatibility)
- [x] 5.3 Import a fixed OSM extract with the change disabled (linear scan) and enabled (dispatch) and verify the generated database files are byte-identical (spec: import-type-resolution/Backward compatibility)
- [x] 5.4 Measure import Preprocess time for a fixed input with 671 vs 1527 types and document the improvement; verify the +47% type-growth penalty is eliminated or reduced to noise (spec: import-type-resolution/Resolution cost independent of type count)
- [x] 5.5 Check new/modified code against `.uncrustify` and `.clang-tidy` conventions (guidelines/CodeStyles.md) and update TODO.md if the change reveals further pre-existing issues
