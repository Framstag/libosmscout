## 1. Build System Preparation

- [ ] 1.1 Add `catch2MainDep` to WorkQueueTest and AsyncProcessingTest in `Tests/meson.build` where currently using plain math/openmp deps — ensure Meson links Catch2WithMain (1 SP)
- [ ] 1.2 CMake `osmscout_test_project` macro already provides `Catch2::Catch2WithMain` for default TARGET — no change needed for these 2 tests (0 SP)

## 2. Convert WorkQueueTest

- [ ] 2.1 Replace `int main()` with `TEST_CASE("WorkQueue")`, keep Worker class and task loop unchanged (3 SP)
- [ ] 2.2 Collect future results and add `REQUIRE(future.get() == expected)` for all 100 tasks (2 SP)
- [ ] 2.3 Remove `std::cout` progress prints or move behind `INFO()` macro (1 SP)
- [ ] 2.4 Verify via `ctest -R WorkQueueTest` (1 SP)

## 3. Convert AsyncProcessingTest

- [ ] 3.1 Replace `int main()` with `TEST_CASE("AsyncProcessing")`, keep IntWorker, VectorWorker, ProcessingQueue classes unchanged (3 SP)
- [ ] 3.2 Add `REQUIRE(worker.processedCount == iterationCount)` after each worker's Wait() completes (2 SP)
- [ ] 3.3 Remove `std::cout` progress prints or use `INFO()` (1 SP)
- [ ] 3.4 Verify via `ctest -R AsyncProcessingTest` with increased timeout (1 SP)

## 4. Verification

- [ ] 4.1 Build with CMake + Meson, run full test suite, confirm no regressions (3 SP)
- [ ] 4.2 Check that non-converted tests (MultiDBRouting, ThreadedDatabase, TextLookup, OSTAndOSS) still build and run with their custom main() unchanged (1 SP)

**DROPPED** (keep custom main for manual CLI invocation):
- MultiDBRoutingTest — coordinates + DB path as args
- ThreadedDatabaseTest — threads/iterations + DB path + style path
- TextLookupTest — expected-results + DB path + query
- OSTAndOSSTest — warning-as-error + ost/oss paths
