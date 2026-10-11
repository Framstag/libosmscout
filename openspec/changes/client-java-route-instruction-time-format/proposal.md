# Proposal

## Why

A route description prints a per-step time of less than a minute as "0 min". Every step of a city route is
therefore shown as taking no time, so the step list and the analysed step carry no usable duration, although
the routing engine estimated one.

## What Changes

- A description line SHALL print its per-step time in hours and minutes when it is at least a minute, in
  seconds when it is at least a second and below a minute, and SHALL print no time at all when it is below a
  second.
- The same rule SHALL apply to every path that produces a description line, so a recalculated route prints
  the same times as a calculated one.
- Distances and the other columns of a description line SHALL be unchanged.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `route-description`: a per-step time below a minute is printed in seconds instead of as "0 min", and a
  per-step time below a second is printed as no time.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — the description text builder of both route-building paths
  (calculate and async calculate/reroute) now calls the shared helper.
- `libosmscout-client-java/src/route_step_time.h` — new, the time rule itself, dependency-free so a host test
  can exercise it (the pattern the client already uses for `search_scope.h`).
- `Tests/src/RouteStepTimeTest.cpp` — new host test, registered in `Tests/CMakeLists.txt` and
  `Tests/meson.build`.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI or Java signature change. No change to the per-step numeric values published on the route
or the instruction; only the text of a description line changes.

Consumers: every caller that displays description lines; a city route's steps now show a duration instead of
"0 min".
