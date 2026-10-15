# Design

## Context

See `proposal.md` - Why. This section is the state the design starts from.

The step is `OptimizeAreaWayIdsGenerator` (`libosmscout-import/src/osmscoutimport/GenOptimizeAreaWayIds.cpp`,
declaration in `libosmscout-import/include/osmscoutimport/GenOptimizeAreaWayIds.h`). It runs as step #11
of the import and consists of four phases over two inputs, all through `FileScanner`/`FileWriter`:

```
  Phase 1  ScanAreaIds   (:264)  areas2.tmp  parse every area, per ROUTABLE RING a fresh
                                 container (:301) of the ring's node ids, then fold it into
                                 `usedIdSet` / `usedIdAtLeastTwiceSet` (:309-315)
  Phase 2  ScanWayIds    (:332)  wayway.tmp  same per ROUTABLE WAY (:368), plus the back id of a
                                 circular way forced into the at-least-twice set (:389-390)
                                 (the two scans are the same data, different files)
  Phase 3  CopyAreas     (:56)   areas2.tmp  parse and write areas3.tmp, clearing the serial of
                                 every node id not in the at-least-twice set (:134); recompute the
                                 ring centre via `OptionalRingCenter` (:76, :138)
  Phase 4  CopyWays      (:166)  wayway.tmp  parse and write ways.tmp, same clearing (:221)
  Phases 3 and 4 run in parallel when the toolchain has std::execution (:445-462)
```

Facts that constrain the design:

- **The decision has a per-ring and per-way notion of distinctness, not a global one.** Phase 1 builds the
  container inside the ring loop and phase 2 inside the way loop, so an id referenced twice inside one
  ring, or twice inside one non-circular way, counts as referenced **once** and its serial is cleared.
  Dropping the per-object container without replacing that notion would silently change the output
  (it would keep serials for self-touching rings, which then also changes `areaarea.idx`/`areaway.idx`
  size downstream). The rule is pinned by the `Node serial clearing rule` requirement in the spec.
- **Phases 3/4 cannot be fused into phases 1/2.** The set that decides phase 3 depends on phase 2 as
  well (an id used once by an area and once by a way must keep its serial), and phase 4's decision
  depends on phase 1. Both copy phases also need the parsed object to write it, so no phase can be
  removed by buffering ids alone.
- **The measured baseline** (`maps/Dortmund.txt`, step #11): `=> 10.718s, RSS 320,8 MiB`, with
  349,965 areas and 78,792 ways, `335,601` relevant nodes and `94,697` of them referenced at least
  twice; `295,176` serials cleared in the areas file and `2,342,587` in the ways file. Phase 1 and
  phase 2 each report their own counts (`:320`, `:395`).
- **Cost attribution is not yet measured.** The baseline log gives totals, not a split across the four
  phases, and neither `usedIdSet` (`:413`) nor `usedIdAtLeastTwiceSet` (`:414`) is given a capacity,
  so both rehash while they grow. Task 1 of `tasks.md` produces the split before the rework is fixed;
  D1 and D4 say what the measurement decides.
- The two inputs are produced by `MergeAreasGenerator` (`GenMergeAreas.cpp`, `AREAS2_TMP`) and
  `WayWayDataGenerator` (`GenWayWayDat.cpp`, `WAYWAY_TMP`) and written with
  `Area::WriteImport`/`Way::Write`, so a fixture test can produce them with the library itself.

## Goals / Non-Goals

Goals (design level):

- The per-ring/per-way distinctness survives the removal of the per-object container, provably:
  a unit test drives the rule with the four shapes the shipped data rarely contains.
- The written bytes stay identical, proved by a fixture test rather than by the Dortmund import alone.
- The step's cost is attributable per phase after this change, so the next import change does not have
  to start from a step total again.

Non-Goals:

- No change to what the step decides, and no change to the set of ids it reports.
- No lighter reader for the scan phases and no format change to `areas2.tmp`/`wayway.tmp` (D2, C).
- No parallelism change: whether phases 3/4 keep running in parallel is left as it is.

## Decisions

### D1 - How the per-object distinctness is computed without a container per object

*Chosen: a reused scratch buffer of the object's node ids, sorted and deduplicated before it is folded
into the global sets (variant C).*

The attribution of task 1.2 (`verification.md`) measured both scan phases together at 0.37-0.51 s of a
5.99-7.02 s step, and the serial clearing in the two copy phases at 0.33-0.47 s. Variant C removes both
the per-object container and the second hash probe per node reference from the two scan phases, which
is the larger half of that 0.9 s; variant A removes only the container and keeps the double hashing.
The measurement therefore picks C, as the design already preferred, and D1 is unchanged in substance -
only its size relative to the step is now known.

- **A - Hoist the existing container out of the loop and `clear()` it per ring and per way.**
  Smallest diff, semantics unchanged by construction, one bucket array reused for the largest object.
  Risk: every node reference is still hashed twice (once into the object container, once into the
  global set), and `clear()` keeps the bucket array of the largest ring for the rest of the scan. Cost:
  the double hashing over 2.3 M references stays. Kept as the fallback if the measurement of task 1
  shows that `std::sort` over the small per-object id lists costs more than the second hash.
- **B - One stamp map (`id -> last ring/way counter`) plus one set of ids referenced at least twice.**
  No per-object container at all, one hash probe per node reference. Risk: a map node is allocated per
  distinct id (335 k allocations today) where a set node is allocated per relevant id; the map is
  strictly larger than the two sets it would replace, and the ring/way counter has to be fed through
  both scans, coupling the rule to the phase order. Rejected.
- **C - Reused `std::vector<Id>` scratch: copy the object's ids, `std::sort`, `std::unique`, fold
  into the global sets.** One allocation for the lifetime of the step (the scratch grows to the largest
  object), one hash probe per *distinct* id of an object, no hashing for the dedup itself, and the
  object's `idCount` (`:307`, `:374`) is `unique`'s result. Risk: `std::sort` over a few dozen ids per
  object has to be cheaper than the second hash probe; the ids within a ring are not guaranteed sorted,
  so the sort cannot be skipped. Mitigation: it is exactly what task 1's measurement decides between
  C and A, and the byte-identity test is the acceptance gate for either.

### D2 - How many phases the step keeps

*Chosen: keep the four phases (two scans, two copies) and only remove the cost inside them.*

The attribution of task 1.2 corrects the assumption this decision was taken with: the two scan parses
cost 0.37-0.51 s together, not a dominant share, and the floor of the step is `PolygonCenter` in the
area copy phase (4.8-5.5 s), which is out of this change's scope (see Risks). The four-phase shape
stays, and D2's variant C (an id-only scan reader) is left as a follow-up with its size now known to be
at most 0.5 s.

- **A - Chosen.** The dependency in `Context` makes fusing impossible without buffering objects, and
  the copies must parse their input to write it in any case. The remaining floor is the two scan
  parses; task 1 measures whether they dominate, in which case a lighter scan read is a *follow-up
  change* with its own format analysis (C below), not part of this one.
- **B - Buffer the per-object distinct ids collected in the scans in a flat array (ids plus an
  offset/length per object) and use the buffer instead of the per-object container in the copies.**
  Rejected: it removes no phase - the copies still parse each object to write it - it adds a
  second data structure of 2.3 M ids (~18 MB) beside the two sets, and it makes the phase order a
  precondition of correctness rather than a detail.
- **C - Fuse phase 1 and phase 3 (or 2 and 4).** Rejected: phase 3's decision needs phase 2's result.
  A fused phase could only write serials that phase 2 may still invalidate. A cheap id-only scan
  reader (skip the coordinate data of an object) would attack the floor of A, but it couples the step
  to the binary layout of `areas2.tmp`/`wayway.tmp` and needs its own verification that the skipped
  bytes are decoded identically - out of scope here, recorded as a follow-up.

### D3 - Where the rule lives and how it is tested

*Chosen: extract the rule into a small type in the import library (private header under
`libosmscout-import/include/osmscoutimport/private/`), unit-tested directly; a fixture test covers the
files the step provides.*

- **A - Chosen.** The four shapes that the rule has to get right (id twice in one ring, id twice in one
  non-circular way, id shared between an area and a way, first id of a circular way) are cheap to drive
  directly and expensive to construct through the pipeline. `Tests/src/MapPainterAreaPreparationTest.cpp`
  is the local precedent for a small object plus assertion style; `WaterIndexTest` is the precedent for
  a test that links `OSMScout::Import` only.
- **B - Keep the rule inside the step's `.cpp` and test it through fixture files only.** Rejected: the
  fixture needs a `TypeConfig` with routable types, an `ImportParameter` with a destination directory
  and the exact `areas2.tmp`/`wayway.tmp` encoding, so the special shapes would either be missing from
  the tests or make the fixture brittle against an unrelated format change. The fixture test stays, but
  for the byte-identity requirement only.

### D4 - The improvement target, and what fixes it

*Chosen: the change aims at the part of the step it can reach - the id decision, measured at 0.9 s of
the 5.99-7.02 s step - and states that explicitly instead of aiming at the whole step. The earlier
target of 0.8 x the step duration was dropped: the measurement puts the reachable ceiling of this
change near 13 % of the step, and keeping 0.8 x would have meant either abandoning the change or
quietly weakening the scenario, both of which the owner of the change rejected.*

*Measured outcome (task 5.3, `verification.md`): the wall-clock effect of this change is **not
resolvable** on this machine. The addressable share is 0.4-0.5 s of a 6-7 s step and the machine's
throughput varies by ~20 % between runs, so the change is verified by its allocation contract (a
constant allocation for 10 000 objects, where the replaced implementation allocated a container per
object) and by the unchanged output, not by a duration. The three fastest after runs are below the
before median and the medians overlap; no win is claimed.*

- **A - Chosen.** The spec deliberately carries no numeric threshold (it requires the measurement and
  the comparison), so the target can follow the measurement without changing a requirement. Keeping the
  number in the design is what makes it revisable with evidence.
- **B - Fix 0.8 x in the spec.** Rejected: the baseline split is not known yet, and an unreachable
  number in a requirement would either block the change or invite a silently weakened scenario at
  archive time.

### D5 - How the byte-identity test obtains its inputs

*Chosen: the test writes `areas2.tmp` and `wayway.tmp` in a temporary directory with the library's own
writers (`Area::WriteImport`, `Way::Write`), runs the step, and compares the produced files against
files produced by a reference run of the same objects.*

- **A - Chosen.** No binary fixture in git, no dependence on a hand-written encoder, and the test stays
  valid if an unrelated area/way field changes. `Tests/src/DbJsonWriterTest.cpp:47` and
  `Tests/src/StyleConfigSymbolsTest.cpp:74` are the precedent for `TESTS_TMP_DIR` in a test.
- **B - Commit recorded `areas2.tmp`/`wayway.tmp`/expected-output files.** Rejected: binary fixtures in
  a text-oriented test tree, and an encoder change elsewhere silently invalidates them.
- **C - Compare against the real Dortmund import before/after.** Rejected as the *only* check: it is
  the cost measurement, it needs a 20 MB extract and minutes of runtime, and it cannot be run in the
  ordinary test job. It stays the verification of the cost requirements (task 5).

### D6 - What the reference implementation of the rule for the test is

*Chosen: the fixture test keeps a small reference implementation of the current rule (per-object
container, fold into two sets) inside the test file, and the test asserts that the step's output
matches the reference's output.*

- **A - Chosen.** It makes the "unchanged files" requirement a comparison against the *meaning* of the
  old rule instead of a frozen blob, and the reference's shapes are visible next to the assertions.
  Precedent: the tile-data-conversion change kept its replaced conversion in a test for the same
  purpose.
- **B - Assert against hand-written expectations per object.** Rejected: the expectations would have to
  encode the whole file, which is what A produces mechanically.

### D7 - Which reader and writer the test uses for each file

The test found that `areas3.tmp` is the *database* format, not the import format: the step writes it
with `Area::Write` and the step that consumes it (`GenAreaAreaIndex.cpp:579`) reads it with
`Area::Read`, while `areas2.tmp` and `wayway.tmp` use `WriteImport`/`ReadImport`. *Chosen: the fixture
and its reference use exactly the readers and writers the pipeline uses for each file.* The difference
is observable - only the database writer stores the ring centre (7 bytes per ring) - so a reference that
used the other pair produced a 35-byte-shorter file for the five-ring fixture. Variants: writing both
sides with `WriteImport` (rejected, it does not describe the file the step provides) and decoding the
serials instead of comparing bytes (rejected: it cannot cover the ring centre and the untouched fields).

## Flow

```
Before                                        After

areas2.tmp --parse--> [ring container]--+     areas2.tmp --parse--> [scratch vector: sort+unique]--+
wayway.tmp --parse--> [way container]---+--> sets                                                      +--> sets
   (2 x per-object container)                 (1 reused buffer, 2 x per input)
                                          |
areas2.tmp --parse--> clear serials --> areas3.tmp; ring centre
wayway.tmp --parse--> clear serials --> ways.tmp
   (unchanged phases 3/4; can run in parallel)

Cost split the design assumes (task 1 confirms or refutes):

  phase 1 + 2 = parse 430k objects + hash 2.3M references twice + 430k containers
  phase 3 + 4 = parse 430k objects + hash 2.6M lookups + write 430k objects + 350k ring centres
```

## Files and modules that change

| Path | Change |
|---|---|
| `libosmscout-import/src/osmscoutimport/GenOptimizeAreaWayIds.cpp` | phases 1/2 use the extracted rule; per-object container replaced (D1); the two global sets are reserved from the counts the scan phases report; the phase split is reported |
| `libosmscout-import/include/osmscoutimport/GenOptimizeAreaWayIds.h` | only if the rule's declaration belongs on the step's header; prefer a private header (D3) |
| `libosmscout-import/include/osmscoutimport/private/` (new header) | the rule: per-object distinctness plus the two global sets, `ClearSerial` decisions |
| `libosmscout-import/CMakeLists.txt`, `libosmscout-import/meson.build` | the new header/source, in both build systems |
| `Tests/src/OptimizeAreaWayIdsTest.cpp` (new) | rule tests (D3/A), fixture byte-identity test (D5/A, D6/A) |
| `Tests/CMakeLists.txt`, `Tests/meson.build` | registration of the new test (the two lists have diverged before, see `TODO.md`) |
| `Tests/src/HeaderCheckTest.cpp` | the dependency allowlist of the new package `osmscoutimport.private` (`=> osmscout`, `=> osmscoutimport`) |
| `openspec/changes/improve-optimize-area-way-ids/verification.md` (from the apply workflow) | the before/after phase split, the A/B comparison and the end-to-end hash comparison |
| `maps/Dortmund.txt` | unchanged; it is the recorded baseline the comparison references |
| `TODO.md` | the entry "The dominant import step constructs a hash set per object and parses every area and way twice" gets its closed-by annotation |

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| D1 changes the clearing rule invisibly (self-touching rings, ways that visit a node twice) | the rule tests of D3/A cover exactly those four shapes; the spec pins them |
| D1 fails to reach the 0.8 x target because the parse/write floor is higher | task 1 measures the split first, D4 allows a revised target that is recorded with the measurement, and the spec requires the comparison rather than a number |
| The extracted rule becomes a second place that knows the ring/way distinctness | it is the *only* place after the change; phases 1/2 call it, and the copy phases keep reading the resulting set |
| A fixture test that writes `areas2.tmp` couples the test to the encoders of `Area`/`Way` | the writers are the library's own (`WriteImport`/`Write`), and an encoder change would break the real pipeline too; the test failing is then the intended signal |
| Registering the new test in one build system only | tasks.md registers it in `Tests/CMakeLists.txt` and `Tests/meson.build` and runs both rosters (the known divergence in `TODO.md`) |
| Reserving the global sets from a count that is not known before the scans finish | reserve from the report of the first scan phase and let the second grow it; the reserve is a hint, not a contract |
| The change's reachable win is ~13 % of the step, and 82-86 % of the step (the ring centre computation) is out of its scope | the measurement is recorded in `verification.md` and in the change's goal (D4) instead of being hidden behind a target the change cannot reach; the ring centre is reported to the owner as a separate matter and is not absorbed into this change |

## Migration Plan

None. The step's inputs, outputs and the bytes it writes are unchanged, so no database has to be
re-imported for correctness, no format version changes, and the change is revertible by reverting the
commits. The Dortmund comparison is a development-time measurement, not a migration step.

## Open Questions

- Whether the two scan phases can be made cheaper by reading only the ids of an object (D2, C) is
  deliberately left open; it needs its own format analysis and does not change this design.
