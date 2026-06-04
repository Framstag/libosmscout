## Why

6 test programs in the test suite still use hand-rolled `main()` functions and custom assertions vs the Catch2 test framework. 4 of these (MultiDBRoutingTest, ThreadedDatabaseTest, TextLookupTest, OSTAndOSSTest) use complex CLI args (CmdLineParsing) for manual invocation with different params — their custom main() is intentional. 2 (WorkQueueTest, AsyncProcessingTest) are self-contained with no CLI args and suitable for Catch2.

## What Changes

- Convert `WorkQueueTest.cpp` to use catch2 TEST_CASEs and REQUIRE assertions
- Convert `AsyncProcessingTest.cpp` to use catch2 TEST_CASEs and REQUIRE assertions
- Remove custom `main()` functions from both converted files — let Catch2 provide the main entry point
- Remove custom assertion/result-tracking patterns in favor of REQUIRE/CHECK macros
- Update meson.build if any test target configuration changes are needed

**Out of scope** (explicitly NOT converting, keep custom main):
- MultiDBRoutingTest — complex CLI args (coords + DB path) for manual invocation
- ThreadedDatabaseTest — complex CLI args (threads/iterations + DB path + style)
- TextLookupTest — complex CLI args (expected-results + DB path + query)
- OSTAndOSSTest — complex CLI args (warning-as-error + ost/oss paths)
- Tests flagged as SKIPTEST or manual-only (ClientQtThreading, DrawTextQt, QtFileDownloader, LaneEvaluation)
- Performance-only tests (CachePerformance, NumberSetPerformance, ReaderScannerPerformance, CalculateResolution, CoordinateEncoding, PerformanceTest)

## Capabilities

### New Capabilities
- `test-conversion-workqueue`: Convert WorkQueueTest to Catch2
- `test-conversion-asyncprocessing`: Convert AsyncProcessingTest to Catch2

### Modified Capabilities

None — no spec-level behavior changes, only test framework migration.

## Impact

- **Affected code**: 2 test source files in `Tests/src/`
- **Build system**: Minor — Catch2 already a dependency. Meson needs catch2MainDep added for these 2 targets
- **No API/ABI changes**: Tests only, no public library surface affected
- **CI**: No change — all tests execute the same way via CTest/Meson test runner