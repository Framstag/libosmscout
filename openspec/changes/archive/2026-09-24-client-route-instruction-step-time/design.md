# Design

## Context

See `proposal.md` - Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp` contains the `JavaRouteInstruction` struct, a
  `JavaRouteInstructionBuilder` and a `CollectCallback` that implements
  `osmscout::RouteDescriptionPostprocessor::Callback`. The callback is invoked once per route node
  (`BeforeNode`) and once per instruction kind (`OnStart`, `OnTurn`, `OnRoundaboutEnter`, ...), and it builds
  one `JavaRouteInstruction` per instruction with `instr.distanceTo = distance.AsMeter()`, where `distance`
  is the node's distance from the route start.
- `RouteDescription::Node` carries both a distance and a time; the callback currently keeps only the
  distance. The route description's own step summary is derived from those node times.
- The Java object is constructed by the bridge through a `GetMethodID` constructor lookup whose signature
  string is a literal, followed by `NewObject` with the field values in order. A mismatch between that
  signature string and the Java constructor fails at runtime, not at compile time.
- `RouteInstruction` is a data class: the bridge produces it, and the Java API also offers a short
  constructor for callers that need to create one (for example in a test or a stub listener).

## Goals / Non-Goals

**Goals:**

- Carry the estimate that the route description already computes to Java, without new route computation
  and without changing the route description or the navigation engine.
- Keep the field's meaning unambiguous: it is a time, in seconds, of a segment, and 0.0 means unknown.
- Cover the Java contract with a test that does not need a map database, a native library or a route.

**Non-Goals:**

- Changing `RouteInstructionAgent`, the `PositionAgent` or the description postprocessor.
- Changing the JavaScout UI to display the time (the capability's UI requirements are untouched).
- Re-deriving a remaining-time estimate from the current position (see D4).
- Extracting the downstream branch's abscissa-based distance refinement: upstream's builder is called
  without an abscissa, so that path has no caller here.

## Decisions

**D1 - The time comes from the description's per-node times, as the difference between consecutive nodes.**
Alternatives:
- *Derive the time from the instruction's distance and an average speed*: the bridge has the route's total
  distance and duration, so it could scale - but that average ignores the per-type speeds the routing
  profile applies (motorway against residential), so the number would disagree with what the route
  description and the routing itself report.
- *Read a pre-rendered step text from the description*: the textual form ("[1.2 km, 5 min]") is presentation
  and would have to be parsed back into a number.
Chosen because the node times are the description's own source of truth, are already visited by the
callback, and need no new data or API.

**D2 - The value is truncated to whole seconds.**
Alternatives:
- *Keep the fractional duration*: it would imply a precision the routing estimate does not have.
- *Report milliseconds*: a unit mismatch next to a distance in meters, and a second unit for callers to
  convert.
Chosen because the route description's granularity is seconds and a client that displays "in 5 min" or
"45 s" needs no more.

**D3 - The full constructor gains the parameter; the short constructor keeps its signature and reports 0.0.**
Alternatives:
- *Add a second, longer constructor and keep the current full one unchanged*: no break for external Java
  code, at the cost of two nearly identical constructors that can drift apart in their field handling and
  defaulting.
- *Make the time a mutable field or a setter*: the class is otherwise immutable (all fields `final`) and the
  bridge produces it fully populated in one call.
Chosen because the class describes one complete instruction, the artifact is a `1.0-SNAPSHOT` with no
in-repository caller of the full constructor, and the short constructor remains for callers that have no
time. The break is stated in the proposal.

**D4 - The next instruction reports the time of its whole segment, not a remaining time.**
Alternatives:
- *Scale the segment time by the remaining fraction of the segment*: it would make the next instruction's
  distance and time agree, but it invents a number the description does not provide, and the estimate is not
  linear in distance because the profile's per-type speeds change within a segment. Upstream's builder is
  also called without the position's abscissa, so the fraction would have to be approximated from
  coordinates.
- *Subtract the already-travelled time using the vehicle's current speed*: it would mix the routing estimate
  with a measurement and make the field's meaning depend on when it is read.
Chosen because the field then always means the same thing ("the estimated time of this segment"), and a
client that wants a remaining time can scale it against its own remaining distance and speed. The
inconsistency with the next instruction's remaining *distance* is recorded in the risks and in the PR.

**D5 - The per-step time is derived in the collector, as the nodes are visited.**
Alternatives:
- *A second pass over the description after collecting*: it would duplicate the node traversal and the
  `stopAfter` window logic, and would have to reconstruct which node preceded which instruction.
- *Compute it in the builder from the collected list*: the collected list holds instructions, not nodes, so
  the node times would have to be carried along anyway.
Chosen because the callback already sees every node in order and can keep the previous node's time in one
member.

## Sequence diagram

```
RouteDescriptionPostprocessor
        |  per node
        v
CollectCallback::BeforeNode(node)
        |   prevTime = time; time = node.GetTime(); distance = node.GetDistance();
        v
CollectCallback::OnStart/OnTurn/OnRoundaboutEnter/... (per instruction)
        |   instr.distanceTo = distance.AsMeter();
        |   instr.timeTo     = SegmentTimeSeconds();   // (time - prevTime) in whole seconds
        v
JavaRouteInstruction  --(collect list)-->  JavaRouteInstructionBuilder
        |                                          |
        |                                          v
        |                       NewObject("<init>", "(DDL...;...)", distanceTo, timeTo, ...)
        v
Java RouteInstruction { distanceTo, timeTo, ... }
```

## Risks / Trade-offs

- *The full constructor change breaks external Java code that calls it* → the short constructor is
  unchanged, the artifact is a `1.0-SNAPSHOT`, and there is no in-repository caller; the alternative (a
  second constructor) is described in D3 and can still be chosen in review.
- *The next instruction's time and distance do not describe the same span* (D4) → the meaning of the field is
  documented in the spec and the field is not silently scaled; a client that wants a remaining time for the
  next manoeuvre must scale it itself, and a follow-up change could add that with an explicit name.
- *Truncation to whole seconds drops up to one second per step* → acceptable for an estimate that a client
  displays as "45 s" or "5 min"; the route description has the same granularity.
- *A JNI signature string mismatch fails only at runtime* → the signature string and the `NewObject`
  argument list were changed together and the Java constructor was verified against them; the Java-level
  test constructs the object through the same constructor, so a parameter-order mistake in Java is caught.
  The bridge path itself cannot be exercised without a map database, which the repository's Java CI does not
  provide (the DB-driven Java tests are skipped there); this gap is stated in the tasks.
- *The callback's `time` member is only advanced in `BeforeNode`* → an instruction emitted without a
  preceding node for the same step reports the delta from the previous node, which is what the description's
  own per-step text uses; no instruction kind in the callback can be emitted without a node.

## Migration Plan

No data migration: nothing persisted or computed changes, and the routing and navigation code is untouched.
A Java caller that constructs the full constructor must pass the new argument; a caller that uses the short
constructor needs no change. Rollback is a revert of the commits, after which the field disappears again and
the bridge signature string matches the old constructor.

## Open Questions

- Whether the JavaScout UI should display the time (for example "300 m, 45 s") - deferrable, it is a UI
  change in a different capability and does not affect this contract.
