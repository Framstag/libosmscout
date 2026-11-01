# Proposal

## Why

The JNI bridge of the Java client prints positions verbatim from its legacy route
entry: the bounding box it checks against a coordinate, the coordinate a
`GetClosestRoutableNode` lookup succeeded at, and the start and destination of a
calculation — on both the resolving line and the failure line.
`GeoCoord::GetDisplayText()` and `GeoBox::GetDisplayText()` render a coordinate pair,
and these lines reach Logcat under the application's own tag, so they are coordinate
log lines. A coordinate in a log is a location trace; identity of the operation, a
count or a distance is enough to diagnose the same failure.

## What Changes

- The database-box trace line names the database path it checked instead of the box.
- Each `ResolveRoutePosition` success line names the search radius (the distance unit)
  instead of the coordinate that resolved.
- The start and destination trace lines, and their failure lines, name the route end
  they are resolving instead of the coordinate.
- No behaviour changes: the entry is the second, legacy route entry the Android app
  does not call, and the seven tracing statements are the only information removed.

## Capabilities

### Modified Capabilities

- `client-java-jni-binding`: the contract between the Java client's `native`
  declarations and the JNI functions now also covers what a bridge log line may
  contain — no coordinate pair, only identity, counts and distances.

## Impact

- `libosmscout-client-java/src/OSMScoutClient.cpp` — seven tracing statements in the
  legacy route path (`[JNI] checking db box`, `ResolveRoutePosition: found via coord`,
  `calculateRouteAsync: resolving start/dest`, and the two failure lines).
- No Java API change, no JNI signature change, no database or file-format change.
