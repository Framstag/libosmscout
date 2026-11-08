# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: both route entry points run their work inside
  `data->dbThread->RunSynchronousJob(...)`, whose callback receives the `databases` instance list. When that
  list is empty the branch sets `errorKind = naviveylin::kRouteFailureRoutingDataUnavailable` and returns.
- `openDatabases` does not populate that list itself: it publishes the set change to the DB thread's job
  queue, and the job clears and repopulates the list later. A calculation that starts before the job ran
  therefore sees the empty list — a retry-clearable condition, not a map-data one.
- `route_failure_kind.h` holds the bridge's own cause vocabulary and its translation to the kind numbers the
  Java `RouteCallback` declares. The translation is total (every cause has a branch), and
  `Tests/src/RouteFailureKindTest.cpp` pins both directions, so a kind added on one side without the other
  fails the host test.
- The two entry points are separate copies of the same pipeline (`AGENTS.md`: a change to one belongs in
  both), so both `databases.empty()` branches carry the same rule.

## Goals / Non-Goals

**Goals:**

- Give the not-yet-loaded list its own kind, distinct from the unusable-routing-data kind, at both entry
  points.
- Keep the number identical on the Java and the native side, and keep the cause→kind mapping total.

**Non-Goals:**

- Changing when `openDatabases` registers the set change, or making a route calculation wait for it (a
  synchronisation change, not a reporting one).
- Reclassifying the branches where the routing service cannot be opened: those keep the
  unusable-routing-data kind.
- Distinguishing "no database was ever asked for" from "the open has not run yet": with the current
  publish-then-return shape, an empty list does not carry that distinction.

## Decisions

**D1 — The empty instance list gets its own kind (7).**
A calculation that finds no instance because the queued set change has not run reports
`kRouteFailureDatabaseNotLoaded` / `KIND_DATABASE_NOT_LOADED`, not the unusable-routing-data kind.
Alternatives:
- *Keep the unusable-routing-data kind*: a caller cannot tell a retry-clearable condition from a map-data
  one, which is the defect this change removes.
- *Rename the existing kind*: it is also the kind of a database that cannot be opened, which is a different
  fact and must stay reachable.
Chosen because the two conditions are different facts for a caller — one is cleared by trying again, the
other by changing the map data.

**D2 — The number is 7, after `KIND_ENGINE_FAILURE` (6).**
The constant is appended, so every existing number keeps its value; the integer is what crosses the
boundary. Alternatives: renumbering would break a native and a Java side built at different times. Chosen
because appending is the only non-breaking choice.

**D3 — The cause vocabulary gains the member next to `RoutingDataUnavailable`.**
`RouteFailureCause::DatabaseNotLoaded` sits after `RoutingDataUnavailable` and before `EngineFailure`, and
`RouteFailureKind` gains its branch; the host test's cause array grows to eight. Alternatives: reusing
`RoutingDataUnavailable` would not record the new fact in the bridge's own vocabulary. Chosen because the
mapping stays total and the test sees a cause without a branch.

## Risks / Trade-offs

- *A caller that switches on the kind does not know 7 yet* → it treats it as `KIND_UNKNOWN`, the documented
  behaviour for an unknown kind; the constant is additive.
- *The condition is timing-dependent and hard to hit in a test* → the host test pins the cause→kind mapping
  and the numbers, and the branch itself is compile-verified; the route path needs a routable v27 database.

## Migration Plan

Reporting-only change: no JNI signature, no constructor, no type and no data change. Rollback is a revert of
the commit, which restores the unusable-routing-data kind for the empty list.

## Open Questions

- Whether `openDatabases` should register the set change synchronously, so the window does not exist at all.
  That is a synchronisation question, deferrable, and out of this unit's scope.
