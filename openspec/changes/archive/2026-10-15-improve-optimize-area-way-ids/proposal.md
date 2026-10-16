# Proposal

## Why

The step that optimizes the node ids of areas and ways is the largest single cost of a full import:
10.718 s of the 28.5 s Dortmund import (37.6 %) and 320.8 MiB of resident memory
(`maps/Dortmund.txt`, step #11). Its cost follows the number of imported objects and node references,
although the decision it produces - which node ids keep their serial - depends only on how often each
id is referenced. It also processes each of its two inputs more often than producing its output
requires. Every statement about import time in `TODO.md` rests on a baseline this step dominates, and
the object growth of the parked type set feeds straight into it, so it has to be reworked before the
cost of unparking those types can be measured.

## What Changes

- The step reaches its id decision without a container that exists only for one ring or one way, and
  without examining its inputs more often than the decision and the written output require.
- The rule the step applies stays exactly as it is today: which node serials are cleared and which are
  kept, including the distinctness of ids inside one ring or way and the exception for circular ways.
- The two temporary files the step provides stay byte-identical to what the current implementation
  writes for the same input.
- The step's duration and peak resident memory are measured against the recorded baseline as part of
  this change, and the measurement is recorded with the change.
- The decision rule becomes observable on its own, so that a test can pin it without the full step,
  in addition to a test that compares the provided files.
- No file format, no public API, no import parameter and no downstream consumer changes.

## Capabilities

### New Capabilities

- `import-id-optimization`: the decision which node ids keep their serial during the import, and the
  cost at which the import reaches that decision - the rule, its exceptions, the unchanged files it
  provides, and the cost of the decision relative to the objects it processes.

### Modified Capabilities

None. `import-type-resolution` covers the resolution of an object's type from its tags, which is
untouched here: this change starts from the objects the resolution has already produced.

## Impact

- `libosmscout-import/src/osmscoutimport/GenOptimizeAreaWayIds.cpp`,
  `libosmscout-import/include/osmscoutimport/GenOptimizeAreaWayIds.h` - the step itself.
- `libosmscout-import/include/osmscoutimport/` - the exported decision rule, if the design extracts it
  from the step (see design.md, D3).
- `libosmscout-import/CMakeLists.txt`, `libosmscout-import/meson.build` - the new or moved source of
  that rule, in both build systems.
- `Tests/src/OptimizeAreaWayIdsTest.cpp` (new), `Tests/CMakeLists.txt`, `Tests/meson.build` - the unit
  tests of the rule and of the provided files, registered in both build systems.
- `libosmscout-import/src/osmscoutimport/GenMergeAreas.cpp` and
  `libosmscout-import/src/osmscoutimport/GenWayWayDat.cpp` - only as the writers of the `areas2.tmp`
  and `wayway.tmp` inputs a fixture test has to produce in the format of those steps.
- `maps/Dortmund.txt` - the recorded baseline (`maps/Dortmund.txt` step #11) the step's cost is
  compared against; `TODO.md` - the entry this change closes.
- Downstream consumers of the provided files (`GenCoordData`, the node/area/way data generators and
  the indexes they feed) are unaffected as long as the provided files stay identical.

## Non-Goals

- Unparking any of the parked type definitions, and any change to the type set.
- A generic A/B import harness for arbitrary extracts; this change measures its own step on the
  recorded baseline extract only.
- Reducing the number of phases the step needs (design.md, D2, records why that is not available).
