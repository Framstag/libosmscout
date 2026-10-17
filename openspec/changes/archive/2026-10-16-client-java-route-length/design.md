# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: both route-building paths assign
  `totalDistance = result.GetOverallDistance().AsMeter()` before the description is generated. That figure is
  set from the start/target spherical distance in `AbstractRoutingService.cpp` (the router's cost limit and
  the progress denominator), and the upstream demo prints it as an "Air-line distance".
- The description callback (`DescCallback` in both route-building paths) already tracks the description's
  cumulative distance at the node last visited; on master it is only used to print per-segment line text. It
  is the description's own total, and with `client-java-route-instruction-metrics` it is the sum of the
  per-step legs that change publishes. This change keeps the largest value seen as that total.
- The same call builds the polyline it publishes, so a length of that geometry is available without another
  routing or database access.
- The published length is displayed by the app's route card; the app also sums the step list, so an
  under-counted figure and the step list visibly disagree.

## Goals / Non-Goals

**Goals:**

- Make the published length a length of the route, so it agrees with the drawn geometry and with the step
  list.
- Keep a usable length available even when the description produced nothing.
- Keep the duration consistent with the published length.

**Non-Goals:**

- Fixing the router's under-counted overall distance (a routing-side defect, left for its own change).
- Changing the cost limit, the progress denominator or the router's use of the air-line estimate.
- Changing the route geometry, the description text or the per-step values.
- Adding a new field: the existing `distance` field is corrected.

## Decisions

**D1 — The description's own total has precedence.**
When the description produced an own total — the cumulative distance at its last node — the published length
is that total. On master the per-step leg arrays do not exist yet, so the node total *is* the figure the step
list is built from (the description lines' segments cover exactly that distance); once
`client-java-route-instruction-metrics` lands, that same total is published as the sum of the per-step legs
and the two are equal by construction.
Alternatives:
- *The sum of the per-step leg arrays*: the same quantity, but the arrays do not exist standalone; waiting for
the companion change would make this change unmergeable now. The node total is preferred and stays correct
when the arrays arrive.
- *The router's figure*: the defect.
- *The polyline length always*: it tracks the description to within 0.4 % on the measured routes, but the
  caller also sums the step list, and two figures that differ by a fraction are still two figures.
Chosen because the node total makes "published total = what the step list covers" exact, which is what a user
can check, and because it needs nothing the companion change has not yet merged.

**D2 — The published geometry is the fallback.**
When the description produced no length but the call publishes more than one geometry point, the published
length is the great-circle length of that geometry.
Alternatives:
- *The router's figure as the fallback*: the defect, and the reason this change exists.
- *Report no length (0)*: a caller that falls back to the total would show nothing although a usable polyline
  was published.
- *Query the routing service for the length again*: another routing-side figure that does not match the
  published geometry.
Chosen because the published geometry is the witness that tracks the description, and it is already in hand.

**D3 — The duration follows the published length.**
Alternatives:
- *Keep a duration derived from the air-line estimate*: two figures of one route would disagree about the
  same thing.
- *Recompute the duration from the description*: the description's times are already summed into the duration
  where it exists; the change only keeps the two consistent.
Chosen because a duration that contradicts the length it is shown next to is the same class of defect.

**D4 — The router's under-count is out of scope.**
The change corrects the published value and the log; it does not touch the routing algorithms.
Alternatives:
- *Fix the router's overall distance as well*: a much wider change across the routing core, and the routing
  figure legitimately serves the cost limit and the progress denominator, which a route length is not.
- *Silently clamp the router's figure somewhere in the routing stack*: hides the defect and can break the
  progress denominator.
Chosen because the published-length contract can be satisfied without touching routing, and the routing
defect deserves its own analysis.

## Sequence diagram

```
calculate route            description callback              polyline builder            Java
     |                            |                               |                       |
     | description ------------>  | keep the largest cumulative   | totalDistance = its   |
     | no description ----------> | distance (= its own total) --->| own total, else 0     |
     |                            | (no total) ------------------>|                       |
     |                            |                               | count > 1?             |
     |                            |                               |   yes -> totalDistance|
     |                            |                               |          = polyline   |
     |                            |                               |   no  -> 0            |
     | getOverallDistance() ------| (never read into the published length)                |
     | duration from the published length --------------------------------------------------> RouteEntry
     | success log names the air-line estimate as such --------------------------------------|
```

## Risks / Trade-offs

- *A caller that compared route lengths between versions sees larger values* → the previous value was an
  under-count of the geometry the same call published; the values are the defect being fixed.
- *The polyline fallback measures the published geometry, not the routed ways* → it is the geometry the caller
  draws and lists, and it tracks the description; the requirement says "a length of that route".
- *A very short route with a single geometry point reports no length* → documented; there is no geometry to
  measure.
- *The description's sum depends on `client-java-route-instruction-metrics`* → without that change the
  description's own cumulative total is preferred, which is the same intent and still not the air-line
  estimate.
- *The success log changes* → it is a log line that named a non-length a length; the requirements do not
  cover logs, and the change corrects the wording.

## Migration Plan

Value-only change to one field (and the duration that follows it): no signature, no type and no data change.
Rollback is a revert of the commits, which restores the air-line estimate as the published length.

## Open Questions

- Whether the router's overall distance should be corrected or renamed in the routing API: deferrable, and it
  is a routing-side decision.
- Whether the published length should also be offered as "geometry length" for a caller that wants to compare
  it: deferrable, no caller asked for it.
