# Design

## Context

See `proposal.md` — Why. The relevant current state on master:

- `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteInstruction.java` carries the manoeuvre
  as `distanceTo`, `timeTo`, `legDistance`, `turnType`, `streetName`, `description`, `shortDescription` and
  the optional "next next" hint (`nextNextDistanceTo`, `nextNextTurnType`, `nextNextDescription`,
  `nextNextShortDescription`). Two constructor arities exist: the 5-argument one and the full one with 11
  parameters (the per-step values and the hint).
- `libosmscout-client-java/src/OSMScoutClient.cpp` builds those instructions in the
  `JavaRouteInstructionBuilder`'s callback from a `RouteDescriptionPostprocessor` walk
  (`OnRoundaboutEnter` / `OnRoundaboutLeave` among its callbacks), caches the Java constructor method id in
  `GetNavigationListenerMethods` and instantiates a `RouteInstruction` per instruction in
  `CreateJavaRouteInstruction` through one `NewObject` call with 11 arguments.
- libosmscout has both quantities as structure. `RoundaboutEnterDescription::IsClockwise()` and
  `RoundaboutLeaveDescription::IsClockwise()` carry the ring's circulation direction;
  `RoundaboutLeaveDescription::GetExitCount()` carries the exit taken, because
  `RoutePostprocessor::roundaboutCrossingCounter` counts the exits the route passes and hands that count to
  the leave description. It is that ordinal — not the number of exits the ring has, which no part of the
  description carries.
- On the way to Java the two quantities survive only in text: the leave callback composes the English
  `"Take exit N onto X"` / `"Exit N"` from the count, and the entering callback writes no direction at all.
  A consumer that needs the exit or the circulation direction has to parse a string that no consumer can
  translate and that never carries the direction.

## Goals / Non-Goals

**Goals:**

- Carry the roundabout manoeuvre of an instruction as structure: the ordinal of the exit taken and the
  ring's circulation direction.
- Carry the same ordinal for the "next next" hint's own manoeuvre, which can be a different roundabout.
- Keep every existing Java caller compiling and behaving as before, and keep the instruction's text
  byte-identical.
- Keep the JNI constructor descriptor the bridge looks up in step with the Java class that declares it.

**Non-Goals:**

- Changing the instruction text, the turn types, the distances or the hint rule that decides when a hint is
  attached.
- Reporting the ring's total exit count, an exit diagram or a lane-level roundabout model — none of them is
  in the description and none of them was asked for.
- The build-then-allocate dispatch restructure and the pending-exception clearing of the JNI lookups: both
  are the sibling change `client-java-jni-lookup-resilience`, which is stacked on this one and states that
  it edits the same descriptor and its comment.
- Any database, map, style or file-format change: the fields are derived from the in-memory route
  description, so no `FileFormatVersion.md` bump applies.
- Consuming the new fields (an instruction card that picks an exit diagram) — that is a client-side change
  in its own unit.

## Decisions

**D1 — The exit ordinal is the leave description's exit count, published as `roundaboutExitNumber`.**
`RoutePostprocessor::roundaboutCrossingCounter` counts the exits the route passes in circulation order and
that count is the exit taken, so `RoundaboutLeaveDescription::GetExitCount()` *is* the 1-based ordinal. A
non-roundabout instruction and an entered-but-not-left ring report 0.
Alternatives:
- *Parse the ordinal out of the description text*: the text is English-prose composed by the bridge, a
  consumer cannot translate it, and an unparseable text ("Exit ?" for a node without a leave description) is
  indistinguishable from an unknown exit. A structure field is the point of the change.
- *Re-count the crossings in the bridge*: duplicates `RoutePostprocessor`'s counting, and the bridge sees one
  node at a time, while the count is a property of the whole ring traversal.
- *Name the field `exitCount`*: three other quantities in the code base carry that name and two of them mean
  something else (`CrossingWaysDescription::GetExitCount()` is the exits at one crossing node, and the
  Compose roundabout symbol's `exitCount` is the ring's total). The field's javadoc names them so the next
  reader does not repeat the confusion.
Chosen because it is the value the description already computes, published under a name that cannot be
mistaken for the ring's size.

**D2 — One field carries the circulation direction, set by either roundabout callback.**
`roundaboutClockwise` is set from `RoundaboutEnterDescription::IsClockwise()` when the instruction enters the
ring and from `RoundaboutLeaveDescription::IsClockwise()` when it leaves; it defaults to `true`, which is the
value a non-roundabout instruction reports.
Alternatives:
- *Two fields, one per callback*: an instruction is either an enter or a leave, never both, so the second
  field would always be meaningless.
- *Only on the leave instruction*: the entering instruction is what a consumer shows while the vehicle is on
  the ring, and it is the point where the direction is first needed; the entering description carries it, so
  dropping it would lose a value that is in hand.
- *No default*: a Java `boolean` cannot be absent, and every non-roundabout instruction would report a
  direction the requirement then has to call meaningless; `true` is stated as the default.
Chosen because the direction belongs to the manoeuvre being shown, and both the entering and the leaving
node have it.

**D3 — The next-next hint's ordinal is its own field, `nextNextRoundaboutExitNumber`.**
The hint is a manoeuvre of its own: two roundabouts in a row would otherwise be indistinguishable, and the
hint's ring can differ from the ring the current instruction leaves. The field is 0 when the hint is absent
or when the hint's manoeuvre is not a roundabout exit.
Alternatives:
- *Reuse the current instruction's `roundaboutExitNumber` for the hint*: wrong whenever the two manoeuvres
  are roundabouts of their own, which is exactly the case the hint exists for (a roundabout exit close behind
  another).
- *A nested hint object (a `RouteInstruction` inside the instruction)*: a new JNI-visible type and a new
  constructor descriptor, far more machinery than one ordinal; the existing hint fields are already flat
  scalars beside the instruction.
Chosen because it mirrors the existing flat hint fields and needs one argument, not a new type.

**D4 — The full Java constructor gains the three values as trailing parameters; the two shorter arities
delegate.**
The full constructor (14 parameters) assigns all three. The arity without the next-next ordinal (13
parameters) delegates with `nextNextRoundaboutExitNumber = 0`; the arity that predates the roundabout fields
(11 parameters, the one master's callers and tests use) delegates with `0, true, 0`.
Alternatives:
- *Replace the full constructor with the 14-parameter one*: breaks every existing Java caller and this
  library's own tests for a value they do not name; an API change without a need.
- *Default arguments*: Java has none.
- *A builder for the instruction*: a wider change to a type whose instances are created by the native side in
  one call; the delegating overloads keep the call sites as they are.
Chosen because the added data is optional for old callers and mandatory for the bridge, and delegation states
the "no roundabout" default exactly once per arity.

**D5 — The bridge's `instructionCtor` descriptor names the 14-parameter constructor.**
`GetNavigationListenerMethods` looks the constructor up by descriptor and the value passed to `NewObject` has
one argument per Java parameter, so the descriptor's three trailing characters (`IZI`) and the argument order
have to match the Java signature character for character. The verification record keeps the `javap -s` line
of the built jar beside the string in the C++ file, because a mismatch is not caught by
`scripts/check-jni-signatures.sh` (it checks native method declarations, not `<init>` descriptors) and
surfaces as an unusable instruction list at runtime rather than at compile time.
Alternatives:
- *Look the constructor up by parameter types instead of a descriptor string*: JNI has no such lookup;
  `GetMethodID` takes the descriptor.
- *Derive the descriptor at build time from the Java class*: no tool in the build does that today, and a
  generated descriptor would have to be regenerated in both build systems for every constructor change.
- *Leave the old descriptor and add a second construction path*: two ways to build the same object, and the
  bridge would silently keep using the one without the roundabout data.
Chosen because one descriptor for one constructor is the existing pattern, and the check that it matches is
recorded rather than assumed.

**D6 — The defensive dispatch and lookup changes stay out of this unit.**
On this branch a descriptor that matches nothing leaves `instructionCtor` null, and — as the sibling change
`client-java-jni-lookup-resilience` documents — the exception `GetMethodID` leaves pending aborts the next
JNI call, while `DispatchRouteInstructions` still allocates the array for the input size. Both are that
change's subject; this unit only makes the descriptor correct. The comment above `instructionCtor` therefore
states the descriptor requirement and nothing about the dispatch.
Alternatives:
- *Port the sibling's restructure here as well*: the same lines would be changed by both changes, and the
  sibling is already written against the roundabout descriptor; the unit boundary keeps one subject per PR.
Chosen because the data-carrying change is reviewable on its own, and the resilience change names this one as
its stack base.

## Sequence / data flow

```
RouteDescription (ring traversal)                    JavaRouteInstructionBuilder                 Java
        |                                                    |                                |
        | RoundaboutEnterDescription::IsClockwise() --> OnRoundaboutEnter:                       |
        |                                              roundaboutClockwise                       |
        | RoundaboutLeaveDescription::GetExitCount() --> OnRoundaboutLeave:                       |
        | RoundaboutLeaveDescription::IsClockwise() --> roundaboutExitNumber / roundaboutClockwise|
        | (text composed from the same count, byte-identical)                                     |
        |                                                    |                                |
        | hint: next.nextNextRoundaboutExitNumber = it->roundaboutExitNumber (the hint's own)     |
        |                                                    |                                |
        |                                        CreateJavaRouteInstruction                        |
        |                                        NewObject(cls, ctor,                            |
        |                                          distanceTo, timeTo, legDistance,               |
        |                                          turnType, streetName, description,              |
        |                                          shortDescription, nextNextDistanceTo,          |
        |                                          nextNextTurnType, nextNextDescription,         |
        |                                          nextNextShortDescription,                      |
        |                                          roundaboutExitNumber,                           |
        |                                          roundaboutClockwise ? JNI_TRUE : JNI_FALSE,     |
        |                                          nextNextRoundaboutExitNumber)                  |
        |                                                    |----> RouteInstruction(14 params) ---> |
```

## Risks / Trade-offs

- *The descriptor and the Java class drift apart (a native library and a jar from different builds)* → the
  `javap -s` comparison is recorded in `verification.md`; a mismatch is a runtime failure, so
  `client-java-jni-lookup-resilience` (stacked on this change) is what turns it into a missing instruction
  list. Both changes edit this descriptor and its comment.
- *A consumer that reads only strings sees a field it does not use* → the text fields are unchanged, which
  the Java tests assert alongside the new values.
- *`roundaboutClockwise` on an instruction without an ordinal is not meaningful* → stated in the field's
  javadoc and in the requirement; the default `true` is the value such an instruction has always implicitly
  had.
- *The hint carries an ordinal but no direction of its own* → the hint's manoeuvre is almost always the
  leaving of the same ring, and its direction is the current instruction's; a hint that leaves a second ring
  would need its own direction field, which is left as an open question rather than guessed here.
- *No end-to-end roundabout navigation test on this machine* → `maps/` holds format v26 while the library
  expects v27, so no routable database can be opened here; the model is covered by the Java tests and the
  JNI contract by the `javap` comparison, and the gap is recorded in `verification.md`.

## Migration Plan

Additive Java API: three fields, one full constructor arity and two delegating arities that keep the previous
two arities' behaviour. The native side fills the values when the route description carries them. No
database, map, style, file-format or JNI entry-point signature changes, so nothing has to be re-imported and
no caller has to change. The one coupling is the constructor descriptor: the jar and the native library of a
client must come from the same build, as before. Rollback is a revert of the commit, which restores the
11-argument constructor and the old descriptor for both sides.

## Open Questions

- Should the "next next" hint also carry its own circulation direction? The ordinal is the value the hint
  needed (two roundabouts in a row); a hint that leaves a different ring than the current instruction would
  need the direction too, and nothing in this change can tell whether that occurs.
- Should the Android instruction card (or JavaScout's own card) consume the new fields — pick a "take the Nth
  exit" diagram from the ordinal and mirror it for a counter-clockwise ring? Deferrable; no consumer is
  changed here.
- Should `RoundaboutLeaveDescription::GetExitCount()` be renamed in libosmscout to say "ordinal of the exit
  taken"? It is a core-library API change outside this unit, and the field's javadoc carries the meaning
  instead.
