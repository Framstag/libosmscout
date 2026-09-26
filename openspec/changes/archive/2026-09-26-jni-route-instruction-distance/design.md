# Design

## Context

See `proposal.md` for motivation and `specs/turn-by-turn-instructions/spec.md` for the contract.

Facts that shape the approach (verified on `master` @ 791d39743, and on the downstream reference `origin/naviveylin-local`):

- `libosmscout/include/osmscout/navigation/PositionAgent.h` carries `Position::abscissa` with a comment that names this consumer: "Fraction of the current route segment (routeNode -> routeNode+1) where the snapped position lies ... Lets consumers compute true along-route progress instead of a straight-line distance from the segment-start node (which breaks step-distance math on curves and with cross-track GPS error)." Nothing reads it outside the position agent's own tests.
- `RouteInstructionAgent::Process` calls `builder.GenerateNextRouteInstruction(routeNode, route->Nodes().end(), position.coord)` in two places and never passes the progress.
- `libosmscout-client-java/src/OSMScoutClient.cpp` holds the Java instruction builder, which is a *custom* reimplementation (`JavaRouteInstructionBuilder`) rather than a user of the core route-instruction types:
  - `travelled = GetEllipsoidalDistance(coord, previous->GetLocation())` - the straight-line estimate the abscissa comment warns about - and `next.distanceTo = nextAbs - nodeDist - travelled`;
  - `OnTargetReached` hardcodes `instr.distanceTo = 0.0`, while every other kind of instruction sets `instr.distanceTo = distance.AsMeter()` (the distance of its own node from the route start), and `OnStart` already sets `distance.AsMeter()`.
  - The search for the next instruction walks the collected instructions forward while `it->distanceTo <= nodeDist` and returns an empty instruction when it runs off the end, so a destination that reports 0 behind the current node is skipped: after the last manoeuvre the client receives an empty instruction instead of the arrival.
- `Tests/src/NavigationAgentTest.cpp` (added with the step-distance fix) drives `PositionAgent` and `RouteInstructionAgent<TestInstruction, TestInstructionBuilder>` with a synthetic three-node route, and already asserts `position.abscissa` of 0.25 and 0.5. Its `TestInstructionBuilder` implements the coordinate-based call only, and `MakePositionMessage` builds a `Position` it can extend with progress.
- The downstream reference (`c3d90a178` / `345a15c54` on `naviveylin-local`, PR #1773) fixes the JNI side by adding a second overload *inside the JNI builder* whose coordinate-based sibling re-derives an approximation of the abscissa from the fix, gated by "the fix lies within the segment span" and clamped. Its core half (the position agent's abscissa) is already upstream; only this JNI-side workaround is left.
- The Java navigation tests (`OSMScoutClientNavigationTest`, `OSMScoutClientNavigationLiveTest`) need a routable map directory from the environment (`nav.test.db.dir` / `JAVASCOUT_MAP_DIR`) and compute a route near Dortmund; the local `maps/Dortmund` database serves that purpose during development.

## Goals / Non-Goals

**Goals:**

- Make the progress the position agent already reports reach the instruction builder, without breaking builders that do not consume it.
- Make the Java client's next-instruction distance use that progress, and make the arrival instruction carry its own node distance like every other instruction.
- Cover the core part with the existing synthetic C++ harness (CI) and the client-visible part with a Java test in the established style.

**Non-Goals:**

- No change to `PositionAgent`; the progress is already computed and documented there.
- No move of the Java instruction builder into the core, and no new public core API for the remaining-distance formula.
- Not the other `naviveylin-local` leftovers (abscissa-derived instruction *order*, route/cycle stylesheets, search-scope diagnostics).

## Decisions

### D1: The agent offers the progress, the builder decides

Chosen: `RouteInstructionAgent` passes `position.abscissa` to the builder through an additional four-argument call when the builder accepts it, selected with a C++20 `if constexpr (requires { ... })` guard; otherwise the existing three-argument call is made exactly as before.

Alternatives:
1. Change the builder contract to four arguments unconditionally - rejected: `RouteInstructionAgent` is a public header template, so every external builder would have to change for a feature it may not want; the repository's own `TestInstructionBuilder` and any binding outside `libosmscout-client-java` would break for nothing.
2. Have the agent pass the whole `PositionAgent::Position` - rejected: a larger contract change with the same compatibility problem, and builders that only need the coordinate would have to dig it out.
3. Re-derive the progress inside the JNI builder from the raw coordinate (the downstream approach) - rejected: it duplicates the snapping `PositionAgent` has already done, admits in its own comment that it is an approximation, and silently disagrees with the core when the two derivations differ (cross-track error, folds). The value exists; passing it is both smaller and exact.

Reasoning: the guard is the only place in the repository that needs a `requires`-expression, and it is contained in one function; the alternative of an unconditional signature change trades a two-line guard for a breaking change in a public template.

### D2: The JNI builder uses the progress for the travelled distance, with the old estimate as fallback

Chosen: the Java instruction builder takes the progress, computes `travelled` as the current segment's length times that progress (clamped to the segment length), and keeps the straight-line estimate when the progress is zero, which is what the position agent reports for a segment it could not snap onto.

Alternatives:
1. Always use the progress - rejected: for an off-route or unsnapped position the agent reports 0, which would claim the vehicle is still at the segment's start node and inflate the remaining distance by everything travelled on that segment.
2. Keep the straight-line estimate and only fix the arrival instruction - rejected: that leaves the overstatement on folds and with cross-track error, which is the defect that makes an upcoming manoeuvre read as "0 m" while it is still ahead.
3. Move the formula into a core helper so it can be unit-tested - considered; rejected for this change because it adds a public core API for one expression, and the input it needs (segment length, progress, fallback) is already available to the C++ test, which covers the part that can actually be wrong (whether the progress arrives).

Reasoning: the fallback preserves today's behaviour exactly in the case where nothing better is known, and the clamp guarantees the non-negative distance the spec asks for.

### D3: The arrival instruction reports its own node distance

Chosen: `OnTargetReached` sets `instr.distanceTo = distance.AsMeter()`, the distance of the destination node from the route start - the same rule as `OnStart` and every turn instruction.

Alternatives:
1. Keep 0.0 and special-case the search so the arrival is still found after the last manoeuvre - rejected: the client would still be unable to tell "destination reached" from "at the route start", and the client-visible value would keep contradicting the list's own convention; the defect belongs in the value, not in a search workaround.
2. Report the remaining distance to the destination - rejected: it would make the arrival instruction the only member of the list whose distance is relative, which breaks the contract the per-step-time requirement already states for the list ("its distance SHALL be the distance from the start of the route"), and a client that renders the list as a step list would show a destination distance that shrinks to 0 at the end.
3. Report the distance of the last manoeuvre - rejected: wrong by construction, and it would make two consecutive instructions carry the same distance.

### D4: The JNI builder keeps a single call shape

Chosen: the Java instruction builder implements the progress-aware call only; the coordinate-based wrapper with its re-derived approximation is not ported.

Alternatives:
1. Keep both, delegating from the coordinate-based one with the downstream approximation - rejected: dead code for the in-repository call sites (the agent prefers the progress-aware form) whose only effect would be to preserve an approximation nobody calls.
2. Keep both, with the coordinate-based one ignoring progress but using the straight-line estimate - rejected: two paths whose difference is invisible in the repository, and the second would only ever be exercised by an external caller that does not exist.

## Flows

```
PositionAgent                RouteInstructionAgent           JavaRouteInstructionBuilder (JNI)
-------------                --------------------           --------------------------------
GPS fix
  -> SearchClosestSegment
     sets position.abscissa
     (fraction of routeNode
      -> routeNode+1)
     |-----------------------> Process(PositionMessage)
                                  |
                                  +-- requires { builder
                                        .GenerateNextRouteInstruction(
                                            node, end, coord, abscissa) }
                                        |
                                        +--> yes: GenerateNextRouteInstruction(
                                        |         previous, last, coord, abscissa)
                                        |           segmentLen = |previous -> next|
                                        |           travelled  = segmentLen * abscissa
                                        |                      (clamped; straight-line
                                        |                       fallback when abscissa == 0)
                                        |           distanceTo = nextAbs - nodeDist - travelled
                                        |
                                        +--> no:  GenerateNextRouteInstruction(
                                                  previous, last, coord)   [unchanged]

Arrival instruction, generated while collecting the route description:

  BeforeNode(node)          -> distance = node.GetDistance()   (absolute)
  OnTargetReached(...)      -> distanceTo = distance.AsMeter() (was 0.0)
                                  |
                                  v
  search for the next instruction (distanceTo > nodeDist)
     finds the arrival instead of running off the end of the list
```

## Risks / Trade-offs

- [`requires`-expressions and `if constexpr` in a public template are new to this repository, so a toolchain that does not implement them would fail to compile the header] → Mitigation: C++20 is the project standard with extensions off and all CI toolchains (GCC 11/13, Clang, MSVC 2025, MinGW, Apple Clang) support requires-expressions; the guard is a value-dependent condition in a class template, so the discarded branch is not instantiated for a coordinate-only builder; verify on the local GCC/Clang builds in this change.
- [The distance arithmetic itself lives in the JNI translation unit and cannot be reached without a map, a route and navigation, so CI cannot exercise it - the Java navigation tests need an operator-provided map and skip without one] → Mitigation: the C++ test covers the contract that can be wrong independently (progress arrives, builders without it still work), the Java test covers the client-visible distances against the local `maps/Dortmund` database, and the gap is stated in the change and in the PR rather than hidden.
- [A position that legitimately sits exactly at the start of its segment reports progress 0, which is indistinguishable from "no progress"] → Trade-off accepted: in that case the straight-line estimate is also ~0, so both paths agree; the fallback is only visibly different for an unsnapped position, where the straight-line estimate is the better of the two available answers.
- [Reporting the arrival distance as the node distance relies on the route description's node distances being comparable with the routing result's overall distance for the "full list" scenario] → Mitigation: the scenario asserts a tolerance rather than equality; if the values ever diverged by more than the tolerance, that would be a defect in the route description rather than in this change.
- [The Java and JNI sides of the arrival change move together: a client that already treats a distance of 0 as "arrival" keeps working, one that treats 0 as "at start" gains the distinction the spec asks for] → Trade-off accepted, and no Java-side API changes, so the change is additive for clients.
- [Two branches now touch `OSMScoutClient.cpp` (`jni-basemap-config` and this one), in different regions] → Mitigation: both branch off `master`; whichever lands second rebases, and the regions (builder setup vs instruction builder) do not overlap textually.

## Migration Plan

- Branch off `master` (`origin/naviveylin-local` is behind it and its JNI-side workaround is superseded by D1/D4).
- Order: agent guard + C++ tests → JNI builder progress + arrival distance → Java navigation test.
- No data or database format change, and no Java API change: existing clients keep compiling, and the arrival instruction's distance is the only client-visible value that changes meaning.
- Rollback is a revert of the change; `naviveylin-local` should drop its superseded JNI hunks when it is next rebased.
