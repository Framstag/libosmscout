# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: `DescCallback::BeforeNode` is called once per route node
  (hundreds for a city route) while a description line is emitted only for nodes that carry a description.
  The callback advances its per-step reference in `BeforeNode`, so a step's distance and time are the last
  geometry edge before its manoeuvre, not the leg ending at it.
- The instruction collector produces `JavaRouteInstruction` values with `timeTo` from the description's
  per-node time difference, and no leg length.
- The QML/Qt client's `RouteDescriptionBuilder::MkStep` advances its reference per emitted step, which is the
  rule that makes the steps sum to the route totals.
- The bridge already receives the position's progress along its segment (`PositionAgent::Position::abscissa`)
  and uses it for the remaining distance of the next instruction.
- The JavaScout step list and the route card show the per-step values, so a wrong reference is visible.

## Goals / Non-Goals

**Goals:**

- Make a step's values the values of that step's own leg, so the steps sum to the route's distance and
  duration.
- Give an instruction the length of its leg, so a remaining distance can be related to it.
- Publish, per instruction line, the manoeuvre's position and the leg's distance and time, index-aligned
  with the description lines.
- Make the next instruction's time consistent with its remaining distance.

**Non-Goals:**

- Changing the core route description, the routing algorithms or the cost functions.
- Changing the columnar text of the description lines.
- Changing the route length published on `RouteEntry` (see `client-java-route-length`).
- Adding an instruction type, a new description source or a new text index.

## Decisions

**D1 — The per-step reference advances per emitted instruction, not per route node.**
The description callback advances its reference when it emits a line, for both the calculate and the reroute
path.
Alternatives:
- *Keep advancing in `BeforeNode` and divide by the number of nodes*: the number of nodes per leg is not
  known to the callback, and the values would still be geometry samples rather than leg totals.
- *Compute the leg from the instruction's own route node and the previous instruction's route node
  afterwards*: needs the route node of every instruction in the bridge and repeats arithmetic the callback
  already has.
- *Adopt a different rule than the Qt client's*: two clients would describe one route differently.
Chosen because it matches the reference client and makes the sums exact by construction.

**D2 — The leg length is a field of the instruction.**
`JavaRouteInstruction` gains `legDistance`, filled by the collector (`FillLeg`, one call per emitted
instruction), and `RouteInstruction` gains the field and a full-constructor parameter; the short constructor
keeps its signature.
Alternatives:
- *Derive the leg length from consecutive absolute distances on the Java side*: a rebuilt list has no
  preceding instruction, and the subtraction would be a consumer-side re-derivation of a native fact.
- *Reuse the next-next hint structures*: they describe a following manoeuvre, not the leg ending here.
Chosen because the leg is a property of the instruction and the collector is the only place that knows it.

**D3 — The four per-instruction arrays are published as a set and dropped as a set.**
They are index-aligned with the instruction lines of the description (the header line is not an instruction
and is not counted); if the alignment cannot be shown to hold one-to-one, all four are null and the rest of
the `RouteEntry` is reported as before.
Alternatives:
- *Publish them aligned with all description lines*: a consumer that iterates the instruction lines would
  read every value from the wrong index by one.
- *Publish partial arrays*: a consumer cannot tell which entries are trustworthy, and a shorter array would
  silently shift the alignment.
Chosen because a null array is a clear "unavailable" and a wrong alignment is a silent defect.

**D4 — The next instruction reports the remaining time of its leg.**
The leg's time is interpolated at the current position with the same progress the remaining distance uses.
Alternatives:
- *The whole leg's time regardless of progress*: an arrival estimate would never shrink, and it would
  contradict the remaining distance reported in the same message.
- *The time of the geometry edge ahead*: reintroduces the wrong reference this change removes.
Chosen because consistency between the two values of one instruction is what a caller displays.

**D5 — An unknown leg reports 0.0, not a cumulative value.**
The first instruction of a list rebuilt from the current position reports unknown leg values.
Alternatives:
- *The distance from the route start*: a value that belongs to another leg and would be shown as this
  step's length.
- *A negative sentinel*: no other field of the type uses one, and 0.0 is already documented as "unknown" for
  the per-step time.
Chosen because the type's existing "0 means unknown" convention carries over.

## Sequence diagram

```
route result            DescCallback (per emitted line)         CollectCallback          Java
    |                          |                                     |                     |
    | BeforeNode(node) ------->| collect position of the manoeuvre   |                     |
    |                          | leg = node - last emitted reference |                     |
    |                          | advance the reference here          |                     |
    |                          | instructionDistances/instructionTimes append                |
    |                          |                                     |                     |
    | route instruction ------>|                                     | FillLeg(leg) ------>| legDistance
    |                          |                                     | timeTo (remaining)  |
    |                          |                                     |                     |
    | marshalling: instructionLats/Lons/Distances/Times -------------|-------------------->| RouteEntry
    |             dropped as a set when the alignment check fails    |                     |
```

## Risks / Trade-offs

- *A consumer that compared `timeTo` between versions sees different values* → the values are the defect
  being fixed; the field's documented meaning (leg's own travel time) is the contract.
- *A rebuilt instruction list reports unknown leg values for its first instruction* → documented, and it
  prevents a wrong number; a consumer that wants a length can fall back to its own route geometry.
- *The alignment check can drop the arrays for a description that produces no instruction lines* → all
  four are null and the description-driven fields still describe the route, so nothing else breaks.
- *The Java full constructor gains a parameter* → the short constructor keeps its signature, and the
  argument order is followed by the JNI constructor descriptor in the same step, so a mismatch is a
  compile-time or startup failure rather than a silent one.
- *The change touches the description text path* → the text columns are not changed; only the numbers fed
  into them change, which the step-sum test pins.
- *The alignment and step-sum contract needs a routable database to be observed* → no database of the
  library's format exists in the verification environment, so the test could not be executed there; the
  contract is held by construction and by inspection of the two callbacks instead (see `verification.md`),
  and a route-level test remains desirable once a database is available. A pure value-class test of the
  instruction's own leg runs offline.

## Migration Plan

Additive at the type level for the short constructor and for the description lines; the values of `timeTo`
and of the text columns change, which is the defect being fixed. `RouteEntry` gains four fields that are null
when unavailable. Rollback is a revert of the commits, which restores the per-node reference.

## Open Questions

- Whether the per-instruction arrays should also be published on the reroute path's rebuilt list: it is
  published there; the alignment rule is the same.
- Whether a leg's time should be reported exactly or clamped like the distance: deferrable, and it changes
  no displayed value beyond rounding.
