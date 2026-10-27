# Tasks

## 1. The resolver

- [x] 1.1 Add `libosmscout-import/include/osmscoutimport/private/BoundaryTypes.h` and
  `libosmscout-import/src/osmscoutimport/BoundaryTypes.cpp` with
  `TypeInfoSet ResolveBoundaryTypes(const TypeConfig&, Progress&)`; register the translation unit in
  `libosmscout-import/CMakeLists.txt` and `libosmscout-import/src/meson.build`. Spec:
  *Every declared boundary type is treated as a boundary*.
  Verify: both build systems compile the import library.
- [x] 1.2 In the resolver, resolve `boundary_country`, `boundary_state`, `boundary_county` and
  `boundary_administrative` as required (assert, as before) and `boundary_municipality` and
  `boundary_suburb` optionally, reporting a missing optional type through `Progress::Debug`. Spec: *A type
  definition without the newer boundary types still imports*.
  Verify: case 2.2 of `BoundaryTypesTest` passes and no warning is emitted for an older type definition.

## 2. The two call sites

- [x] 2.1 Replace the six `GetTypeInfo` calls in `GenLocationIndex::Import`
  (`libosmscout-import/src/osmscoutimport/GenLocationIndex.cpp:2761-2783`) with the resolver and drop the
  now unused `boundaryType` local. Spec: *Every declared boundary type is treated as a boundary*.
  Verify: `LocationBoundaryRegionTest` passes; the region of the level-8 fixture is present.
- [x] 2.2 Replace the six `GetTypeInfo` calls in `GenRelAreaDat::ResolveMultipolygonMembers`
  (`libosmscout-import/src/osmscoutimport/GenRelAreaDat.cpp:651-673`) with the resolver and drop the now
  unused local. Spec: *Both steps that read boundary types agree*.
  Verify: the import library compiles; `LocationLookupTest` (which imports through both steps) passes.

## 3. Tests

- [x] 3.1 Add `Tests/src/BoundaryTypesTest.cpp` with the cases "The shipped type definition resolves every
  boundary type it declares" and "A type definition without the municipality and suburb types resolves
  without them" (the latter writes a minimal `*.ost` into `TESTS_TMP_DIR`), and register a
  `BoundaryTypesTest` target in `Tests/CMakeLists.txt` and `Tests/meson.build` with `TESTS_TOP_DIR` /
  `TESTS_TMP_DIR` set. Spec: *Every declared boundary type is treated as a boundary*, *A type definition
  without the newer boundary types still imports*.
  Verify: `ctest -R BoundaryTypesTest` passes; case 3.2 shows the legacy case fails against the
  assert-per-name form.
- [x] 3.2 Probe the red state of the legacy case: temporarily make the resolver assert on
  `boundary_municipality` (the local patch's form) and run `BoundaryTypesTest`; the second case aborts.
  Restore the resolver afterwards and confirm `git diff` carries no probe marker. Spec: *A type definition
  without the newer boundary types still imports*.
  Verify: the abort is captured in `verification.md`.
- [x] 3.3 Extend `Tests/LocationTest.olt` with a level-8 boundary under `Kreis Unna` and a level-10
  boundary nested in it, and add `Tests/src/LocationBoundaryRegionTest.cpp` with the cases "A level-8
  boundary is a region of the location index" and "A level-10 boundary is a region of the location index,
  nested in the municipality"; add the file to the `LocationLookupTest` sources in both build systems.
  Spec: *Every declared boundary type is treated as a boundary*.
  Verify: both cases pass, and they fail with the resolver reduced to the four legacy names.
- [x] 3.4 Check the two new fixture levels for collisions with the names the existing cases of the target
  search for: no existing case names them, and the `LocationIndexRegionCacheTest` cases of the same
  database assert relative counts only. Verify: `ctest -R LocationLookupTest` passes all cases unchanged.

## 4. Build and regression checks

- [x] 4.1 Build the tree: `cmake --build build` and `meson compile -C debug`. Verify: both exit 0, no
  compiler diagnostic for the touched files.
- [x] 4.2 Run the touched targets: `ctest -R 'LocationLookupTest|BoundaryTypesTest' --output-on-failure`
  and the `Check LocationService`, `BoundaryTypes` tests of the Meson build. Verify: no failing case.
- [x] 4.3 Run the surrounding suite for regressions:
  `QT_QPA_PLATFORM=offscreen ctest -j 8 --output-on-failure -E NumberSetPerformanceTest`. Verify: no
  failing test beyond the ones the branch's base already fails.
- [x] 4.4 Check the touched files against the conventions: `uncrustify -q -c .uncrustify -l CPP -f <file>`
  for the five changed/new C++ files, `scripts/format-check.sh` where it applies, plus
  `guidelines/CodeStyles.md` (include order, naming, comments). Verify: no new hunk for the added lines.

## 5. Records

- [x] 5.1 Write `verification.md` beside the proposal: every claim and number the change cites, with the
  command, timestamp, exit code and the evidence excerpt. Verify: no number is unbacked.
- [x] 5.2 Archive the change with `openspec archive fix-boundary-region-types` so the capability spec lands
  in `openspec/specs/import-administrative-boundaries/`. Verify: `openspec validate` reports no error and
  the archived directory carries proposal, design, tasks, verification and the spec.
- [x] 5.3 State in the archived `verification.md` that the relation composition has no end-to-end case and
  why (fixture format generates ways). Verify: the limitation is named once, not silently omitted.
