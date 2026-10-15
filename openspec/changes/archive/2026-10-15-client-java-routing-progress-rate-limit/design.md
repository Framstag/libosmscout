# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: the routing progress implementation's `Progress` is
  called by the router once per successfully relaxed edge, always on the routing worker thread. Every call
  attaches the worker thread to the JVM when needed and invokes the Java callback with a percentage derived
  from the progress denominator.
- The percentage is already capped at 99, and completion is delivered through the route result and the
  success callback, not through a progress value.
- The routing progress value the engine reports grows monotonically, so the denominator-relative percentage
  cannot decrease.
- The Java callback is a `RouteCallback.onProgress(int)`, which the app uses for a progress display.

## Goals / Non-Goals

**Goals:**

- Make the callback mean "the progress moved" rather than "an edge was relaxed".
- Bound the number of JNI crossings per calculation by time, not by graph size.
- Keep the callback's signature and the meaning of its value unchanged.

**Non-Goals:**

- Changing the routing engine's progress reporting or its frequency at the engine level.
- Changing the progress denominator or how the percentage is computed.
- Coalescing or queueing intermediate values so that every percentage eventually reaches Java.
- Changing the cancellation, success or error callbacks.

## Decisions

**D1 — Time-based rate limit combined with change detection.**
A report is handed over only when the percentage changed and the interval since the last handover has passed.
Alternatives:
- *Change detection only*: a long route visits many percentages, so it still floods Java; and on a slow
  graph a change can be a single edge.
- *Report every Nth edge*: the callback rate then depends on the graph's density, not on wall-clock time, so
  a caller cannot size its display against it.
- *Rate limit only, without change detection*: reports an unchanged value every interval, which makes the
  callback mean "time passed", the defect in the other direction.
Chosen because the two conditions together give the callback a meaning a consumer can rely on.

**D2 — Intermediate values are dropped, not queued.**
Only the latest accepted value is handed over; a value skipped by the interval is lost.
Alternatives:
- *Queue the skipped values and drain them*: a progress bar would then play catch-up through values that are
  already stale, and the queue could grow without bound on a long route.
- *Keep a timer that reports the latest value when the interval elapses*: needs a thread or a timer callback
  on the routing side, and it makes the callback arrive after the routing has moved on.
Chosen because progress is a sampled indicator, not a log.

**D3 — The first report goes through immediately.**
The "nothing reported yet" state bypasses both conditions.
Alternatives:
- *Wait for the interval before the first report*: a short calculation would deliver no progress at all, and
  a caller could not distinguish "not started" from "no reports".
- *Report 0 % explicitly when the calculation starts*: duplicates the start-of-calculation signal the caller
  already has, and it would be an unchanged-percentage report by the new rule.
Chosen because a caller should see the first real percentage without an artificial delay.

**D4 — Completion stays out of the progress stream.**
The percentage remains capped below completion; the completed state is the result.
Alternatives:
- *Report 100 % as progress*: a caller could treat a completed-looking percentage as success before the route
  exists, and the cap already exists for that reason.
- *Report completion through a separate callback*: one exists (the success callback), so a second one would
  be redundant.
Chosen because it keeps the existing contract between progress and result.

**D5 — The decision is extracted into a dependency-free header.**
The rules of D1 to D4 live in `libosmscout-client-java/src/routing_progress_throttle.h`, next to the existing
`search_scope.h` precedent: the percentage conversion with the completion cap, and a small class holding the
last handed-over percentage and time whose `ShouldReport(percent, now)` takes the time as a parameter. The
callback in `OSMScoutClient.cpp` keeps the JNI attach and the Java call, which still happen only for an
accepted report.
Alternatives:
- *Keep the rule inside the callback class*: the class is a local of one translation unit and needs a JNI
  environment, the routing engine and a map database to exercise, so the rule would only be observable through
  a route probe — and a timing-dependent one at that.
Chosen because the rule is then covered deterministically on the host, with no routing engine, no JNI and no
map database, and because an injected time is what removes the flakiness a wall-clock test would have. The
state stays in one place: the throttle object is the only holder of the rule's state, and the callback owns
exactly one of it.

## Sequence diagram

```
router (worker thread)          Progress()          RoutingProgressThrottle            Java callback
      |                             |                        |                                |
      | progress(edge) ------------>| percent, now --------> |                                |
      |                             | first report? ---------| accepted ---------------------> | onProgress(p)
      | progress(edge) ------------>| unchanged/lower? ------| dropped (no JNI crossing)      |
      | progress(edge) ------------>| changed in interval? --| dropped (no JNI crossing)      |
      | progress(edge) ------------>| changed, interval past-| accepted ---------------------> | onProgress(p')
      | ...                         |                        |                                |
      | result ---------------------| (completion is not a progress value)                    |
      |                             |                        |  success callback ------------> | onSuccess(route)
```

## Risks / Trade-offs

- *A caller that expected one callback per percentage* → the callback's documented meaning becomes "progress
  moved", and the final value is delivered at the end of the calculation.
- *A very short calculation delivers one or two reports* → the first report goes through immediately, so a
  caller still sees that the calculation progressed.
- *The interval is a fixed constant* → it is documented and testable; a caller that needs a different rate
  has no way to ask, recorded as an open question.
- *The routing thread's clock is read once per edge* → a monotonic clock read is negligible next to a JNI
  attach and a Java call, which is what the change removes.
- *Dropping an intermediate value could make a display jump* → a progress display is a sample; a jump is
  preferable to an unbounded callback storm.
- *The throttling itself is hard to unit-test on the host* (it needs the routing engine and a JNI
  environment) → the decision is extracted into a dependency-free header and covered by a host test (D5); the
  route path itself is only exercised by inspection, because the shipped map databases are one type-config
  format version behind the library and no v27 database is available here (recorded in `verification.md`).

## Migration Plan

No API change: the callback keeps its signature and its value's meaning. The observable difference is the
number of calls and the guaranteed first call. Rollback is a revert of the commits, which restores one
callback per edge.

## Open Questions

- Whether the rate-limit interval should be configurable by the caller: deferrable, no caller asked for it.
- Whether the decision function should be extracted into a dependency-free helper so it can be host-tested:
  settled — it is extracted, see D5. The helper holds the rule's state and the callback holds exactly one
  helper, so the state has a single source of truth.
