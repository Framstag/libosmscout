## Context

The test suite in `Tests/src/` has 48 Catch2-based tests and 6 remaining tests with hand-rolled `main()` functions and custom assertion patterns. Of these 6, 4 use complex CLI args (CmdLineParsing) for manual invocation with different params and are out of scope. 2 are self-contained with no CLI args and suitable for Catch2 conversion.

Each unconverted test falls into one of three structural patterns:

1. **CmdLineParser-based** — parses CLI args, runs computation, returns 0/1 (MultiDBRouting, TextLookup, OSTAndOSS)
2. **Thread-worker based** — spawns threads, verifies results via bool flags or future values (ThreadedDatabase, WorkQueue, AsyncProcessing)
3. **Print-and-return** — prints results to stdout, just returns 0

All tests already link against `OSMScout` libraries. Catch2 is already available as `Catch2::Catch2WithMain` via the `osmscout_test_project` CMake macro.

## Goals / Non-Goals

**Goals:**
- Convert 2 self-contained unconverted test files to use Catch2 `TEST_CASE` macros and `REQUIRE`/`CHECK` assertions
- Remove custom `main()` functions — let Catch2 provide the entry point
- Preserve exact behavioral semantics (same data, same operations, same success/fail criteria)
- Keep test names registered in CTest/Meson exactly as they are

**Non-Goals:**
- Not rewriting performance-only, manual, or interactive tests (CachePerformance, NumberSetPerformance, ReaderScannerPerformance, CalculateResolution, CoordinateEncoding, PerformanceTest, ClientQtThreading, QtFileDownloader, DrawTextQt, LaneEvaluation)
- Not converting tests with complex CLI args designed for manual invocation (MultiDBRoutingTest, ThreadedDatabaseTest, TextLookupTest, OSTAndOSSTest) — they keep their custom main() for flexible manual use
- Not adding new test cases not already present — pure framework migration
- No changes to test data, map directories, or external dependencies
- No changes to the public API or library code

## Decisions

### Decision 1: Each test becomes one TEST_CASE (preserving original name)
**Rationale**: Original tests are monoliths with a single main() execution path. Splitting into multiple TEST_CASEs would require restructuring the test logic, increasing risk of behavioral change. A single named TEST_CASE per file mirrors existing converted tests (e.g., `TEST_CASE("FileScannerWriter")` in FileScannerWriterTest.cpp).

### Decision 2: Custom assertion helpers convert to REQUIRE() directly
**Rationale**: Tests like OSTAndOSSTest use `errorCount` tracking and `return 1/0`. This becomes `REQUIRE(errorCount == 0)`. Tests like ThreadedDatabase use bool results — those become `REQUIRE(result)`. No need for fixture classes.

### Decision 3: Build system changes are minimal
**Rationale**: The `osmscout_test_project` macro defaults to linking `Catch2::Catch2WithMain`. The 2 converted tests (WorkQueue, AsyncProcessing) use the default TARGET and already get Catch2 transitively. In Meson, these tests need `catch2MainDep` added to their dependency list.

## Risks / Trade-offs

| Risk | Mitigation |
|------|-----------|
| **Behavioral change**: Catch2 REQUIRE throws on failure instead of returning 1 — could affect cleanup code | Both tests return immediately on error (no complex cleanup beyond RAII), so REQUIRE is safe. |
| **Thread safety**: Catch2 might not handle threaded TEST_CASEs well | Both tests spawn threads internally — thread logic stays inside the TEST_CASE body. Catch2 does not interfere with user-managed threads. |
| **No CLI args needed**: These tests are self-contained with no external data dependencies | No data path configuration needed — they work out of the box. |
