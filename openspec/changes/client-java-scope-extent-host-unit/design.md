# Design

## Context

See `proposal.md` — Why. The current state that shapes the approach:

- `naviveylin::DeriveScopeExtent` is a `static` function inside `libosmscout-client-java/src/OSMScoutClient.cpp`,
  the single JNI translation unit (see `libosmscout-client-java/AGENTS.md`). The host test suite cannot
  compile that file: it needs JNI headers and the generated JNI method table. A test that reaches the
  derivation therefore has to live outside it.
- The derivation's decision function, `naviveylin::IsInsideGeoBox`, already sits in
  `libosmscout-client-java/src/search_scope.h`, a dependency-free header the host suite compiles
  (`Tests/src/SearchScopeTest.cpp`). Only the derivation — loading the region's object and taking its box —
  is unreachable.
- The moved derivation needs `osmscout::DBInstanceRef` (from `libosmscout-client`) and
  `osmscout::AdminRegionRef` (from `libosmscout`), both of which the host suite already links for other
  tests.

## Goals / Non-Goals

**Goals:**

- Make the extent derivation a unit the host suite compiles and links, with no JNI dependency.
- Move the function without changing its behaviour, its inputs or its result.
- Cover the derivation with a host test that fails if it derives the box from the wrong object.

**Non-Goals:**

- Changing the derivation, the filter, the reported verdict or any search result.
- Moving the scope-expansion rule (`ShouldExpandScope`) or the filter's call sites out of the JNI unit.
- Any JNI signature, database, map or style file format change.

## Decisions

### D1: The unit lives in `libosmscout-client-java/src/search_scope_extent.{h,cpp}`

Alternatives:

- **Keep it in `OSMScoutClient.cpp` and test through a JNI test** — rejected: a JNI host test needs a JVM,
  the generated headers and a built shared library; it would not run in the ordinary `ctest` suite, and the
  reference environment's JavaScout tests go through a fake client that never reaches this code.
- **Put it in `libosmscout-client` (the client library)** — rejected: the derivation is product-specific
  (`naviveylin` namespace, the scope filter's fail-open rule) and does not belong to the shared client
  library's public surface. It also depends on nothing from `libosmscout-client` beyond the `DBInstance`
  handle the caller already holds.
- **Put it in `Tests/`** — rejected: the unit is production code the JNI bridge calls; a test-only
  translation unit would compile it only for the test and leave the bridge's own build with a second copy.

Chosen: beside `search_scope.h`, so both the JNI library and the test compile the same source, and the
`naviveylin` scope helpers stay in one directory.

### D2: The derivation keeps `(DBInstanceRef, AdminRegionRef) -> GeoBox` and does not move the filter

Alternatives:

- **Move the whole filter (the loop that drops out-of-scope hits) out of the JNI unit too** — rejected: the
  filter reads JNI-serialized entry structures and the free-text candidate list, so it cannot leave the
  translation unit without a larger extraction. This change is deliberately limited to the derivation, which
  is the part a test can reach with a database handle.
- **Return `std::optional<osmscout::GeoBox>` instead of `naviveylin::GeoBox`** — rejected: it would change
  the type the call site and the filter use, turning an extraction into a behaviour-adjacent change. The
  existing `GeoBox` with its `isSet` flag already carries the fail-open rule.

Chosen: the function signature and body move verbatim; only the namespace qualification of the name changes
at the call site.

### D3: The test imports its own fixture

Alternatives:

- **Reuse the committed `Tests/data/testregion`** — rejected: its location index is a 77-byte header that
  carries no admin region, so there is no region to derive an extent from. Weakening the case to a region
  the fixture does not hold would not test the derivation.
- **Ship a new committed database** — rejected: a committed database would be a large binary fixture for a
  case that an import of an existing fixture (`Tests/LocationTest.olt`, the one `LocationServiceTest` uses)
  already serves.

Chosen: the case carries its own `main`, imports `Tests/LocationTest.olt` (a full import, not eco, so the
indexes the case asserts against exist), opens the result and runs the cases over it. It removes the
import's `coord.dat` temporary once done.

### D4: Both build systems compile the moved source into the test target

Alternatives:

- **Link the test against the built JNI shared library (`osmscout_client_java`)** — rejected: the test would
  then need the JNI library's own dependencies (Cairo, Pango, a JDK at runtime) and the symbol would have to
  be exported through a hidden-visibility shared object. Compiling the one source into the test keeps the
  test's dependency set to the client and import libraries.
- **A separate small library for the unit** — rejected: one translation unit does not warrant a library, and
  the JNI module already lists it among its sources.

Chosen: `Tests/CMakeLists.txt` and `Tests/meson.build` list `search_scope_extent.cpp` beside the case and
link `OSMScout::Client`, `OSMScout::Test` and `OSMScout::Import` (the test target the fixture import needs).

## Risks / Trade-offs

- [The moved source is compiled twice: into the JNI library and into the test.] → It is one small,
  side-effect-free translation unit; a second compilation is cheap and keeps the test free of the JNI
  library's dependency set.
- [The test imports a fixture at start-up, so it is heavier than a pure unit test and writes to the test
  temp directory.] → It imports into its own subdirectory of `TESTS_TMP_DIR` and removes the coordinate
  spill file, so it neither collides with `LocationServiceTest` nor leaves the multi-hundred-megabyte
  temporary behind.
- [The derivation's node branch (a region represented by a point) is not reachable from the fixture, because
  the import writes no node objects.] → The fallback box it produces is the pure `BoxAroundPoint` function,
  which `Tests/src/SearchScopeTest.cpp` already covers; the test documents this rather than inventing a
  fixture.

## Migration Plan

No data migration. The move changes no interface a consumer sees; the JNI library's public surface and the
Java API are unchanged. Rollback is a revert of the extraction commit.

## Open Questions

None.
