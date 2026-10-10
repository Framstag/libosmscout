# Proposal

## Why

The routing engine reports progress once per relaxed edge, which is thousands to millions of calls for a long
route, and every call crosses the JNI boundary from the routing worker thread into a Java callback. The
callback therefore means "a node was visited", not "the progress moved", and the routing thread spends its
time attaching to the JVM and calling into Java instead of routing.

## What Changes

- A progress report SHALL be handed to Java only when the reported percentage has changed, so the callback
  means "the progress moved".
- Reports SHALL be rate-limited to at most one per documented interval, so a route calculation cannot flood
  the Java thread with callbacks.
- The first report of a calculation SHALL be handed over immediately, so a caller sees progress as soon as it
  exists.
- A reported percentage SHALL be capped below completion, with the completion of the calculation delivered as
  the result rather than as a progress value.
- The reported percentage SHALL be non-decreasing for one calculation, matching the direction of the routing
  progress the engine reports.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-calculation`: the progress a Java caller receives during an asynchronous route calculation is
  throttled, changed-percentage driven and capped below completion.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the routing progress callback that crosses into Java on
  the routing worker thread.
- `libosmscout-client-java/src/routing_progress_throttle.h` — the new dependency-free decision (the change
  detection, the rate limit, the completion cap), so it can be host-tested; the JNI attach and the Java call
  stay in the bridge above and happen only for an accepted report.
- `Tests/src/RoutingProgressThrottleTest.cpp` and its registration in `Tests/CMakeLists.txt` and
  `Tests/meson.build` — the host test of that decision, following the `SearchScopeTest` precedent.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change. No Java source change: the callback's signature and the meaning of its
value (a percentage of the calculation) are unchanged; only how often it is called changes.

Consumers: every caller that passes a route callback; a caller that displays a progress bar sees fewer
callbacks with the same, still-advancing value.
