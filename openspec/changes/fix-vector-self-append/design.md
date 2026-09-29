# Design

## Context

See `proposal.md` for motivation. This section records what the inspection of the tree established,
because it narrows the change to a handful of lines.

**Inventory of the pattern.** A textual search for appends whose argument starts with the element
form of the receiver finds 15 candidates in the tree. Eight of them append an element of the sequence
they append to:

| site | sequence |
|---|---|
| `libosmscout/src/osmscout/util/Transformation.cpp:579` | the optimized sequence of a projected area (closing point) |
| `libosmscout/src/osmscout/util/Transformation.cpp:613` | the same sequence, in the cut-off branch of the simplicity loop |
| `libosmscout/include/osmscout/util/Geometry.h:728` | the by-value parameter copy in `AreaIsSimple` |
| `libosmscout/include/osmscout/util/Geometry.h:878` | the caller's outer ring in `AreaIsValid` |
| `libosmscout/include/osmscout/util/Geometry.h:880` | the caller's inner rings in `AreaIsValid` |
| `libosmscout-import/src/osmscoutimport/WaterIndexProcessor.cpp:1210` | the coastline points of a water index pass |
| `BasemapImport/src/BasemapImport.cpp:247` | the coastline of a way that closes across the antimeridian |
| `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:354` | the ring points of an area in the OpenGL painter's area preparation |

The remaining seven candidates name a different object in the argument than in the receiver
(`GenAreaAreaIndex.cpp:298,372`, `SortWayDat.cpp:219,280`, `WaterIndexProcessor.cpp:615`,
`GenRelAreaDat.cpp:266`, `DbJson.cpp:412`) and are correct as they are; they are listed in the
proposal's Impact section as checked.

**Evidence.** The Release build of this repository (CMake, GCC 16.2.1, `-O3`) reports for
`Transformation.cpp:613`:

```
warning: »*<unbekannt>« könnte uninitialisiert verwendet werden [-Wmaybe-uninitialized]
   110 |         return ::new(__loc) _Tp(std::forward<_Args>(__args)...);
```

reproduced by running the translation unit's own compile command from
`build/compile_commands.json`. Only that one of the eight sites is reported; the Meson build of the
same source reports nothing (recorded in `TODO.md`), so the diagnostic is a symptom, not a gate.

**Storage state of the optimized sequence.** `EnsureSimple` builds its sequence from the points the
optimizer left with a draw flag, one append at a time, so its capacity follows the drawn point count
(1, 2, 4, 8, …). The drawn count therefore decides whether the closing appends grow the storage: at
exactly 4 and exactly 8 drawn points they do. That makes the storage-growth case reachable
deterministically from a test that controls the drawn points.

**Where tests live.** `Tests/src/TransPolygonTest.cpp` holds the three simplicity cases for the
optimizer and reaches the code through `TransformWay`/`TransformArea`. The tree-check precedent is
`scripts/check-jni-signatures.sh`, registered as a test in both build systems
(`Tests/CMakeLists.txt:188` `add_test(NAME JniSignatureParityTest …)`, `Tests/meson.build:14`
`test('Check JNI signature parity', …)`).

## Goals / Non-Goals

**Goals:**

- Every self-append in the tree no longer depends on the storage state of the sequence it grows.
- The optimized geometry is identical to before for every input.
- The storage-growth case has a test, and the pattern has a tree gate that both build systems run.
- The Release build of the library keeps its warning-free state at the optimizer without suppressing
  a diagnostic.

**Non-Goals:**

- No change to the optimizer's algorithm: which points are dropped, the tolerance, the simplicity
  criterion and the constraint semantics stay as they are.
- No new public API, no change of the type definition or database formats.
- The seven cross-container appends are not touched.
- The repository-wide uncrustify drift, the Qt deprecation warnings and the other `TODO.md` entries
  this change does not cause stay out.

## Decisions

### Decision 1: Read the element into a value of its own before the append

Each of the six sites takes the element it wants to append into a value that does not live in the
sequence being grown, then appends that value. In `Transformation.cpp` and the import site that is a
copy of the element; in `Geometry.h` the same, since the helpers append a ring's own first point.

Alternatives considered:

- **Reserve the storage so the append cannot grow.** Rejected: the safety then rests on arithmetic
  between a reserve and every later append in the same function; a later edit that adds a point
  silently removes the guarantee, and the reserve reads like an optimization rather than a
  correctness precondition.
- **One shared helper (for example `AppendClosingPoint(sequence)`).** Rejected: it is a template over
  the six element types for six call sites, it moves the intent away from the place a reader checks it,
  and two of the sites (`Geometry.h`) are header-only templates that the helper would have to be
  visible to.
- **Leave the sites and rely on the compiler diagnostic.** Rejected: the diagnostic covers one of six
  sites and does not appear at all in the Meson build, so it cannot serve as the gate.

Risk of the chosen shape: the copy is easy to undo by a later refactor; Decision 2 covers that.

### Decision 2: A tree check registered as a test in both build systems

A script next to `scripts/check-jni-signatures.sh` reports every append in the tree whose receiver and
whose argument's leading expression name the same object, and is registered as a test in
`Tests/CMakeLists.txt` and `Tests/meson.build` beside the signature check. It reports the receiver and
the argument so a reader can confirm a finding in one line of output, and it fails the test on a
finding.

Alternatives considered:

- **Gate on `-Wmaybe-uninitialized` in a Release build.** Rejected as the primary gate: measured to
  cover one of the six sites and to be absent for the Meson build; it can still be used as a local
  check while implementing.
- **Enable a standard-library debug mode in CI.** Rejected: unverified coverage for this pattern (a
  self-append is not a documented assertion), and a whole-configuration change for a narrow gate.
- **Policy in the style guide only.** Rejected: a written rule is not a gate; `guidelines/CodeStyles.md`
  can still be extended later, but it cannot detect a new site.

### Decision 3: Cover the storage-growth case in the optimizer test

`Tests/src/TransPolygonTest.cpp` gains a case that drives the area optimization with a projected ring
whose drawn point count is exactly the storage capacity of the optimized sequence when it appends the
point that closes the ring for the simplicity decision (four well-separated points, tolerance below
their spacing, driven through the public `TransformArea` path used by the importers and painters), and
asserts that the resulting geometry keeps every point of the ring and is simple as a closed ring. A
second case uses a ring that does not force growth and asserts the same shape contract, so the two
storage states are covered.

Alternatives considered:

- **Compare the two storage states against each other** (a ring that grows against one that does not).
  Rejected: the two rings are different geometry, so the comparison has no meaning; the contract is
  the shape of each result, not their equality.
- **White-box call of `OptimizeArea` with a hand-filled `TransBuffer`.** Viable and equally
  deterministic, but it bypasses the drop steps that decide the draw flags, exactly the input the
  growth case depends on; the public path exercises them, so it is the better home for the case.
- **No test, gate on the check alone.** Rejected: the spec's storage-growth scenario needs coverage of
  its own; the check cannot see whether a geometry is closed.

Known limitation, measured rather than hidden: on the standard library used here the growth case also
passes with the pre-change form of the append restored, because that implementation moves the existing
elements before it constructs the appended one. The case therefore pins the contract; the
discriminating evidence for the defect is the compiler diagnostic and the check. Recorded with it: the
pre-existing case `Optimized area is still simple` fails when the append that closes the ring is
removed altogether, so the decision that append exists for keeps its own regression case and the new
cases do not have to carry it.

## Sequence Diagram

```
TransformArea(nodes) / TransformWay(nodes)                        Transformation.h
      |
      v
TransformGeoToPixel  ->  TransBuffer.points        (draw flag per point)
      |
      v
OptimizeArea(buffer, method, tolerance, constraint)               Transformation.cpp
      |
      |-- DropSimilarPoints / DropRedundantPoints* / DropEqualPoints
      |        (set the draw flags of the surviving points)
      |
      |-- constraint == simple ?
      |        |
      |        v
      |   EnsureSimple(buffer, isArea)
      |        |
      |        |-- build `optimised`: append every drawn point
      |        |        storage grows 1,2,4,8,...   <-- capacity == drawn count at 4 and 8
      |        |
      |        |-- isArea ?  append the first element of `optimised`   <-- SELF-APPEND
      |        |
      |        |-- while (!simple)
      |        |        FindIntersection(optimised, i, j)
      |        |        |-- cut off the smaller part:
      |        |        |     erase [j+1,end), erase [begin,begin+i),
      |        |        |     append first element of `optimised`      <-- SELF-APPEND
      |        |        +-- else: erase [i+1, j+1)
      |        |
      |        +-- write the draw flags back to buffer.points
      |
      +-- buffer.CalcSize()
```

## Risks / Trade-offs

- [A later refactor reintroduces a self-append and the compiler stays silent] → the tree check runs in
  both build systems; the six sites the check currently finds are the check's own fixture.
- [The tree check flags a correct cross-container append] → the check compares the receiver with the
  argument's leading expression and prints both; the seven known cross-container sites are named in
  the proposal so a false positive is recognisable in one line.
- [The check misses a site across lines or behind an alias] → the check is documented as a textual
  gate over the tree, not a proof; the spec's storage-growth scenario and the review of the diff carry
  the rest.
- [The copy changes performance at a hot path] → the sites append once per ring or per coastline, not
  per point; the existing performance tests (`PerformanceTest-*`, the import profile) are the check,
  not a measurement task.
- [The output changes despite the equal semantics] → the existing simplicity cases and the test suite
  are the regression gate; no golden file is expected to move.
- [Only one build system gets the new test] → one task adds the script and both registrations, and the
  known roster-divergence issue in `TODO.md` is explicitly not part of this change.

## Migration Plan

None required. No format, API or generated-artifact change: the change is source-level and the
databases and rendered output are unaffected. Rollback is a revert of the change's commits; the
importers and painters keep working with either revision.

## Open Questions

- Whether the tree check should also cover the `insert` and `emplace` spellings of the same append.
  It changes neither the specs nor the approach nor the task breakdown, so it can be answered while
  implementing the check.
