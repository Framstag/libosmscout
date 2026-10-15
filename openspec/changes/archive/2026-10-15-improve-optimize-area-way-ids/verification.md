# Verification

Measurements taken while implementing this change. Every number names the build it came from, the
command that produced it and the file the log lives in.

## Environment

| Item | Value |
|---|---|
| Build directory | `debug/` - the Meson build whose `Import` produced the recorded baseline (`maps/Dortmund.txt`, "Call: ../debug/Import/Import") |
| Build flags | `-O2 -g -D_GLIBCXX_ASSERTIONS=1 -fopenmp`, `HAVE_STD_EXECUTION` defined (`debug/libosmscout-import/include/osmscoutimport/private/Config.h:18`) |
| Compiler | `c++ (GCC) 16.2.1 20260810`, Meson 1.12.1, Ninja 1.13.2 |
| Host | 16 CPUs, local `nvme0n1p7`, 72 GiB free before the runs |
| Rebuild before measuring | `ninja -C debug Import/Import` (167 edges) so the binary matched the current tree |
| Base of these measurements | `35599b1e4` (master plus the unmerged type-resolution commit). The change was rebased onto master `685461344` for the pull request; both test suites were re-run there (see "Task 5.1 and 5.2") |

## Task 1.1 - Reproduce the recorded baseline

Command (run from `maps/repro-before/`, a scratch directory, so that `maps/Dortmund.txt` and
`maps/Dortmund` stay untouched; `default.opt` was copied there so the option file is read exactly as in
the recorded run):

```
time ../../debug/Import/Import --typefile ../../stylesheets/map.ost \
     --destinationDirectory repro-before ../Dortmund.osm.pbf > repro-before.log 2>&1
```

Result: `Import OK!`, wall time `17,245 s`, whole-run peak RSS `1,0 GiB` at step #3, database 995 MB.

Step #11 reproduced as `=> 5.799s, RSS 254,9 MiB, VM 1,3 GiB` against the recorded
`=> 10.718s, RSS 320,8 MiB, VM 1,3 GiB`.

### The difference is a property of the environment, not of step #11

Per-step comparison of the two runs (`maps/.baseline-Dortmund.txt` is the copy of the recorded file
taken before the reproduction run; `maps/repro-before/repro-before.log` is the new run):

| step | module | recorded | reproduced | ratio |
|---|---|---|---|---|
| #2 | Preprocess | 2.601 s | 0.958 s | 0.37 |
| #3 | CoordDataGenerator | 4.194 s | 1.732 s | 0.41 |
| #7 | WayAreaDataGenerator | 3.195 s | 1.580 s | 0.49 |
| #9 | MergeAreasGenerator | 1.469 s | 0.961 s | 0.65 |
| #10 | WayWayDataGenerator | 1.048 s | 0.754 s | 0.72 |
| **#11** | **OptimizeAreaWayIdsGenerator** | **10.718 s** | **5.799 s** | **0.54** |
| #16 | AreaWayIndexGenerator | 0.337 s | 0.514 s | 1.53 |
| #17 | AreaAreaIndexGenerator | 0.754 s | 0.945 s | 1.25 |
| #19 | WaterIndexGenerator | 0.325 s | 0.456 s | 1.40 |
| whole run | | 28.5 s | 17.2 s | 0.60 |

Every compute-heavy early step is now faster and several late index steps are now slower, so the
recorded profile does not describe this build, this machine or this type set. The recorded absolute
numbers therefore cannot be the target of this change. What the recorded profile still supports is the
*share*: step #11 was 37.6 % of the import then and is 33.7 % of it now, still the largest single step
(next: #3 CoordData 1.732 s, #7 WayAreaData 1.580 s).

Consequences recorded for the change:

- The target of design decision D4 is defined against the *before* measurement of this change on this
  machine (5.799 s for step #11), with the recorded profile used only as a secondary anchor: the after
  run must be faster than the recorded 10.718 s as well.
- The object counts differ slightly between the two runs (`350201` areas vs `349965`, `78827` ways vs
  `78792`, `295275` vs `295176` cleared area serials, `2343996` vs `2342587` cleared way serials) because
  the type set changed between the two dates. Both before/after runs of this change use the same type
  set, so the comparison inside the change is unaffected.
- Step #11's RSS reproduced as 254.9 MiB against the recorded 320.8 MiB, consistent with the smaller
  active type set (the parked types are inactive); the RSS comparison of this change is likewise made
  against the fresh before run.

## Task 1.2 - Attribution of the step's duration

`perf` is not installed on this machine, so the attribution uses per-phase `StopClock` reporting in the
step itself - which is the deliverable task 3.2 owes anyway, and which costs no sampled profiling. Two
runs of the instrumented debug build, same command as above (`maps/repro-before/with-phases.log`,
`phases-1.log`, `phases-2.log`):

| phase | run 1 | run 2 | share of run 2 |
|---|---|---|---|
| scan area ids (`ScanAreaIds`) | 0.244 s | 0.314 s | 4.5 % |
| scan way ids (`ScanWayIds`) | 0.126 s | 0.200 s | 2.8 % |
| copy way data (`CopyWaysProcessor`) | 0.138 s | 0.208 s | 3.0 % |
| copy area data (`CopyAreasProcessor`) | 5.598 s | 6.471 s | **92 %** |
| step total (`=>`) | 5.989 s | 7.020 s | 100 % |

The scan and copy phases sum to the step total (0.314 + 0.200 + 6.472 = 6.986 s against 7.020 s).
Inside the two copy phases, measured per object with a `StopClock` around each sub-step:

| sub-phase | run 1 | run 2 |
|---|---|---|
| area: `data.ReadImport` | 238 ms | 277 ms |
| area: serial clearing (the at-least-twice lookups) | 291 ms | 388 ms |
| area: `OptionalRingCenter` -> `PolygonCenter` | **4835 ms** | **5530 ms** |
| area: `data.Write` | 151 ms | 188 ms |
| way: `data.Read` | 43 ms | 54 ms |
| way: serial clearing | 43 ms | 87 ms |
| way: `data.Write` | 38 ms | 49 ms |

### What this changes about the change

The premise recorded in `TODO.md` for this step - that the per-object container and the double parse
dominate it - is **not supported**: the whole id decision (both scans, 430k per-object containers, 2.3 M
hashed references, 2.6 M lookups while clearing) costs about 0.9 s of the 7.0 s run, and the two copy
phases are 95 % of it. 82-86 % of the step is the single `PolygonCenter` call at
`libosmscout-import/src/osmscoutimport/GenOptimizeAreaWayIds.cpp:86`, run for every ring of every area,
and that is the only `PolygonCenter` call site in the library (the header's other overloads are unused).

Consequences for the decisions in `design.md`:

- D1 (how to keep the per-object distinctness cheap) is still a real improvement, but its ceiling is
  0.9 s of a 7.0 s step: even a perfect implementation cannot reach the 0.8 x target of D4.
- D2's assumption that the two scan parses are the remaining floor is wrong: the scan parses are 0.5 s
  together, and the floor of the step is `PolygonCenter`.
- The improvement this change promised cannot be reached without addressing the ring centre computation,
  which writes into `areas.dat` (`libosmscout/src/osmscout/Area.cpp:472`) and therefore changes database
  content rather than only this step's internal cost.

That is a scope question for the owner of the change, not something to absorb silently; it is raised
before task 1.3 fixes D1 and the target.

## What was implemented

### Task 2.1-2.4 - the rule and its tests

- `libosmscout-import/include/osmscoutimport/private/AreaWayIdReferenceRule.h` and
  `libosmscout-import/src/osmscoutimport/AreaWayIdReferenceRule.cpp`: the rule owns the per-object
  distinctness (a reused scratch buffer, sorted and deduplicated - design D1, variant C), the two sets
  and the forced serial of a circular way. Registered in `libosmscout-import/CMakeLists.txt` (source
  list) and `libosmscout-import/src/meson.build`.
- `GenOptimizeAreaWayIds.cpp` / `GenOptimizeAreaWayIds.h` call the rule from both scan phases and from
  both copy phases. The class got `OSMSCOUT_IMPORT_API`: without it the test cannot instantiate the
  step at all, because the shared library hides the vtable and `Import()` of a generator that no other
  generator exports. `MergeAreasGenerator::AREAS2_TMP` and `WayWayDataGenerator::WAYWAY_TMP` stay
  unexported, so the test repeats those two names locally.
- `Tests/src/OptimizeAreaWayIdsTest.cpp`: 12 test cases. Nine drive the rule; the shapes are an id
  referenced once, an id referenced by two objects, an id shared between an area and a way, an id twice
  inside one ring, an id twice inside one non-circular way, the id a circular way returns to, an id only
  unreachable objects carry, the per-object distinct count and the independence of the object count.
- Registered in `Tests/CMakeLists.txt` (`osmscout_test_project`) and `Tests/meson.build`
  (`executable` + `test`). Both rosters contain it: `ctest --test-dir build -N -R OptimizeAreaWayIds`
  lists test #42, `meson test -C build-meson --list` lists
  `libosmscout:Check the area and way id optimization`.
- `Tests/src/HeaderCheckTest.cpp`: the package dependency allowlist needed the two entries of the new
  package `osmscoutimport.private` (`=> osmscout` for `Id`/`Point`, `=> osmscoutimport` for
  `ImportImportExport.h`); without them the header check reports three violations and the whole
  cross-dependency contract of the import package fails.

### Task 3.1 - variant C and the reserve

Variant C is implemented as designed. The two scan phases call `AddObject()` once per routable ring or
way and get the object's distinct id count back, so the reported counts (`areas, ids found`, `ways, ids,
circular ways found`, `Found ... relevant nodes`) are unchanged. `Import()` reserves the referenced-id
set from the count the first scan reports, as the task asks; the reserve is a hint only, because the
second scan adds an order of magnitude more ids than the first one observed.

### Task 2.5 and 3.3 - sanitizer, formatting and static analysis

- The new test passes under the AddressSanitizer + UndefinedBehaviorSanitizer configuration
  (`build-asan`, 74 assertions in 11 test cases - the allocation case is compiled out there by design)
  both with and without `ASAN_OPTIONS=detect_leaks=0`. The existing import-library tests
  (`WaterIndexTest`, `JsonWriterTest`, `DbJsonWriterTest`) pass.
- `uncrustify --check` with the repository config reports no finding in any of the three new files, and
  none on the lines this change added or changed in `GenOptimizeAreaWayIds.{h,cpp}`. The files keep
  their pre-existing drift elsewhere: the checker still reports 21 hunks in `GenOptimizeAreaWayIds.cpp`
  (member alignment, `Area    data;`, `{ }` bodies, the pre-existing `progress.Info` line) and the whole
  `HeaderCheckTest.cpp` table, all on lines this change did not touch - the drift `TODO.md` already
  records for these files.
- `clang-tidy -p debug` findings in the new code are fixed: the redundant destructor, `contains`
  instead of `find`, the missing direct includes, `std::ranges::sort`/`unique`, the swappable-parameter
  helper, `emplace_back`, the integer division in the fixture and the unnamed loop `push_back`. What
  remains in the new test file are the checks that fire on the repository's own test files with the same
  volume (magic numbers, bounds, `owning-memory`/`no-malloc` from the allocation counter, as in
  `MapPainterAreaPreparationTest.cpp`) plus that counter's `getenv` and unnamed-parameter findings -
  all of it the noise `TODO.md` describes, none of it in the rule itself.
- The test replaces the global allocation operators like `MapPainterAreaPreparationTest.cpp` does. GCC
  reports `-Wmismatched-new-delete` at the sized delete once the objects the fixture builds are inlined
  in the same translation unit; the diagnostic is disabled around the operators with the same
  `#pragma GCC diagnostic` block `MapPainterLabelReuseTest.cpp` uses for it. Without the suppression the
  file compiles with 15 warnings, with it there are none.

### Task 4.1-4.3 - the provided files

The fixture test writes `areas2.tmp` and `wayway.tmp` into `TESTS_TMP_DIR` with the library's own
writers, runs the step, runs a reference implementation of the copy phases kept in the test, and
compares the produced `areas3.tmp` and `ways.tmp` byte for byte. The reference carries the old rule (a
container per ring and per way), the old clearing loop and the old ring-centre rule, so the comparison
covers the decision, the serials, the ring centre and the untouched parts of every object at once.

Two facts the test pinned down while it was written, both recorded here because they are easy to get
wrong again:

- `areas3.tmp` is written in the **database** format (`Area::Write`) and read with `Area::Read` by the
  step that consumes it (`GenAreaAreaIndex.cpp:579`), while `areas2.tmp` is written and read with
  `WriteImport`/`ReadImport`. The two formats differ: only the database writer stores the ring centre,
  which is 7 bytes per ring and exactly the difference the first version of the reference test showed.
  The ways are `Way::Write`/`Way::Read` on both sides.
- A ring of a type that cannot route has no serials written at all (`readIds = CanRoute()`), so the
  fixture assertion for the non-routable area is that its nodes read back with serial 0 rather than an
  assertion about the rule.

The fixture contains the shapes the decision has to get right: an id referenced by one object, an id
shared between an area and a way, an id twice inside one ring, an id twice inside a non-circular way, a
circular way whose back id keeps its serial, an area and a way that cannot route, and - for the centre
rule - an L-shaped ring whose pole of inaccessibility is far from its bounding box centre next to
regular rings that get no centre.

### Task 5.1 and 5.2 - both build systems

- `ninja -C debug` and `ninja -C build` build the whole tree; recompiling the four touched or added
  translation units reports no warning in either build system. The warnings the builds do emit are in
  files this change does not touch (`MapPainterOpenGL.cpp`, `WellScoutedRoute.cpp`, Qt headers).
- `ctest --test-dir build -j 8`: **139 of 139 passed** (`QT_QPA_PLATFORM=offscreen`, `TESTS_TOP_DIR`,
  `TESTS_TMP_DIR` as `AGENTS.md` documents). `meson test -C debug --timeout-multiplier 2 -j 8`:
  **139 OK, 0 fail**. Both rosters include the new test. Re-run after the rebase onto master
  `685461344`, which brings its own vector-self-append check: `ctest` **140 of 140 passed**, `meson test`
  **140 OK, 0 fail**, and both build systems compile the whole tree again without a failure.

### Task 5.3 - the Dortmund comparison, and what it does not show

Clean A/B, same build directory and flags, only the step's implementation differing; the step's own
`=> ...s` line of `maps/Dortmund.txt` step #11, six runs of the reworked step against three of the
pre-change step (obtained by stashing this change, rebuilding and restoring it):

| | before (3 runs) | after (6 runs) |
|---|---|---|
| step #11 duration | 6.271 / 6.318 / 6.324 s (median 6.318) | 5.944 / 5.975 / 6.244 / 6.748 / 6.803 / 7.122 s (median ~6.5) |
| step #11 RSS | 277.1 / 291.3 / 297.0 MiB | 258.1 / 296.8 / 296.8 / 296.8 / 314.6 / 402.6 MiB |
| scan area ids | 0.244 / 0.314 s | 0.246 / 0.246 / 0.249 / 0.267 / 0.266 / 0.288 / 0.375 s |
| scan way ids | 0.126 / 0.200 s | 0.104 / 0.108 / 0.110 / 0.118 / 0.129 / 0.152 / 0.256 s |
| copy area data | 4.8-5.5 s | 5.570 / 5.591 / 5.847 / 6.368 / 6.382 / 6.446 / 6.727 s |
| counts | 350201 areas / 21426 ids, 78827 ways / 421272 ids / 344 circular, 335601 relevant, 94697 at least twice, 295275 + 2343996 serials cleared | identical in every run |

**The wall-clock target is not resolved by this measurement.** The part of the step this change can
reach is the two scans, about 0.4-0.5 s of a 6-7 s step; the machine's throughput varies by roughly
20 % between runs (the copy phase alone spans 5.57-6.73 s without a code change), so a change of the
size this one can deliver is below the noise floor of a single-machine measurement. What the numbers do
show is that the step is not slower: the three fastest after runs are below the before median and the
medians overlap. Reporting a win would be reporting noise, so this change does not claim one.

The peak RSS comparison is likewise unresolved for the same reason: the step's two sets are the same
size in both runs (335601 / 94697), the copy phases write through memory-mapped files, and the
observed spread (258-403 MiB) brackets both runs.

What *is* resolved, and deterministically so, is the contract the change was designed for: the
allocation test shows the rule allocating a constant amount for 10 000 objects with the same ten ids
(11 allocations for the first 1 000, 0 for the next 10 000), where the implementation this change
replaced allocated a container per object. The reachable win (the ring centre, 82-86 % of the step) is
out of this change's scope and is reported rather than absorbed.

### The end-to-end statement about the provided files

The strongest available check of "the provided files are unchanged" is the real extract, not the
fixture: `maps/Dortmund.osm.pbf` imported before and after the change, with the produced `areas.dat` and
`ways.dat` compared from the two runs after the change was re-formatted and linted:

```
b81a629768da2fa332ff09d9d759bf82b3fa65915cd5e1769a50fcb8aecde245  areas-before.dat
b81a629768da2fa332ff09d9d759bf82b3fa65915cd5e1769a50fcb8aecde245  repro-before/areas.dat
19ce8568f31d791bd9678b2f20b72b314c5abaf3d5f774cc121cdc8553461a05  ways-before.dat
19ce8568f31d791bd9678b2f20b72b314c5abaf3d5f774cc121cdc8553461a05  repro-before/ways.dat
```

(identical hashes on both pairs) together with identical object counts and cleared-serial counts in
every run of the comparison above.

### Task 5.5 - documentation

No file in `Documentation/` describes this step, its log output or the import's per-step reporting
(`grep -rl "OptimizeAreaWayIds\|node serials\|Step #1[0-9]" Documentation/ AGENTS.md guidelines/` finds
nothing but `Documentation/Resources.txt`, which names no step), so no documentation statement became
wrong and none was added.
