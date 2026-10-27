# Verification

Iteration §37 of the fix loop, change `fix-location-index-region-cache`. Every number this change quotes in
`proposal.md`, `design.md`, `tasks.md` and `specs/location-index-lookup/spec.md`, the artifact that carries
it, and how that artifact was obtained.

The pin of the frozen revision is the `sha256` of three files, carried by `evidence/gate-cmake-build.log:5-7`:
`c8ad1ebd4be20b6e4c27ee159e2c50da82dc4acd4211ea1657a583ea3ae24ce8`
`libosmscout/include/osmscout/db/LocationIndex.h`, `9115f94628d223d02a6c727af4e1cf1cb8306276014f93658cbcc574811dd608`
`libosmscout/src/osmscout/db/LocationIndex.cpp`,
`fa1fa733d5d63f8e9195ddc5d7a5c1677fce0036342b97f16025781aabdf2806`
`Tests/src/LocationIndexRegionCacheTest.cpp`. The build lists that add the case to the target are pinned
beside them (`f21f5830…` `Tests/CMakeLists.txt`, `6ef70cab…` `Tests/meson.build`,
`evidence/gate-cmake-build.log:8-9`). `evidence/post-fix-test.log:5-7` repeats the three source pins.

## Quoted numbers and their carriers

| number as quoted | carried by | command | run at (UTC) | exit | pin |
|---|---|---|---|---|---|
| red, case "A second traversal…": `REQUIRE( readsOfSecondTraversal==0 )`, expansion `12 == 0`, message "second traversal: 12 regions visited, 12 region entries read" | `evidence/red-test.log:1445-1448`, message `:1432` | `ctest -R LocationLookupTest --output-on-failure` | 2026-10-16T05:52:40Z | 8 | not retained — how obtained: the run measured `master` plus the region-read diagnostic and no other behaviour change; the log carries no source pin and that revision cannot be reproduced by a hash now |
| red, tally: `test cases: 12 | 11 passed | 1 failed`, `assertions: 309 | 308 passed | 1 failed` | `evidence/red-test.log:1450-1451` | same run | same | 8 | same row — not retained, how obtained above |
| red, the two other messages: `first traversal: 12 regions visited, 12 region entries read`, `traversal after a flush: 12 regions visited, 12 region entries read` | `evidence/red-test.log:1431`, `:1430` | same run | same | 8 | same row — not retained |
| after fix, second case count `0`: `second traversal: 12 regions visited, 0 region entries read` | `evidence/post-fix-test.log:1451` | `ctest -V -R LocationLookupTest` | 2026-10-16T06:13:36Z | 0 | the three source pins above |
| after fix, flush-case count `12` and first-traversal count `12`: `traversal after a flush: 12 regions visited, 12 region entries read`, `first traversal: 12 regions visited, 12 region entries read` | `evidence/post-fix-test.log:1449`, `:1450` | same run | same | 0 | the three source pins above |
| after fix, target tally: `All tests passed (309 assertions in 12 test cases)` | `evidence/post-fix-test.log:1453` | same run | same | 0 | the three source pins above |
| probe: `REQUIRE( readsAfterFlush==afterFlushTraversal.regions.size() )`, expansion `0 == 12`, and `REQUIRE( readsOfFirstTraversal==firstTraversal.regions.size() )`, expansion `0 == 12` | `evidence/probe.log:1443-1445`, `:1455-1457` | `ctest -R LocationLookupTest --output-on-failure` | 2026-10-16T05:54:55Z | 8 | none — the run measured the fixed tree with the release line of `FlushCache()` dropped under `TEMP-REVERT-PROBE`; the line was restored before the gate |
| gate, cmake suite: `158/158` | `evidence/gate-cmake-ctest.log:321` | `ctest -j 8 -E 'NumberSetPerformanceTest' --output-on-failure` | 2026-10-16T06:00:03Z | 0 | see `evidence/gate-cmake-build.log:5-9` |
| gate, meson suite: `Ok: 159` / `Fail: 0` | `evidence/gate-meson-test.log:167-168` | `meson test -C debug --timeout-multiplier 2 --print-errorlogs` | 2026-10-16T06:06:49Z | 0 | see `evidence/gate-cmake-build.log:5-9` |
| the revision the gate measured | `evidence/gate-cmake-build.log:5-9` | `cmake --build build -j "$(nproc)"` | 2026-10-16T05:56:22Z | 0 | this row is the pin |

Both suite runs measure the frozen revision, not the probe tree: the probe log predates the gate and its
mutation was restored before `gate-cmake-build.log` was written (`evidence/probe.log:1476-1478`).

## The red row

The count the red case reads has no host seam on pristine HEAD — `LocationIndex::GetRegionReadCount()` arrives
with this change — so the red run measured `master` plus that diagnostic and nothing else. That revision is
not retained and no hash names it. The row above says what the run was instead of presenting it as a
measurement of HEAD; the after-fix and gate rows measure the frozen revision the three pins name.

## Claims this change does not make

- **No concurrent-use contract.** `regionCacheMutex` is a defensive guard: it keeps a caller from observing a
  half-inserted entry or a torn counter. No scenario of `specs/location-index-lookup/spec.md` asserts that
  lookups run on several threads and no case exercises that, and the delta claims nothing of the sort. The
  member comments of the pinned revision (`libosmscout/include/osmscout/db/LocationIndex.h:149-152` for the
  cache members and `:294-299` for `GetRegionReadCount()`) still read as concurrency claims; they are left
  unchanged because narrowing them would change the revision the gate measured (design, "Concurrency").
- **The mutability of a retained region entry is not asserted away.** A caller that writes the region
  `ResolveAdminRegionHierachie` returns would write the retained object, and `AdminVisitor::Visit(const
  AdminRegion&)` blocks mutation only through the visitor. No in-tree caller writes; the out-of-tree case is
  filed as `TODO.md` §105 (design, "Risk").
