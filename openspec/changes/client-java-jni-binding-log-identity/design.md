# Design

## Context

See `proposal.md` — Why. The Java client's legacy route entry
(`Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteWithObjectsAsync`
and the `ResolveRoutePosition` helper and `CollectDescriptionCandidates` callback it
calls) writes `GetDisplayText()` values into `osmscout::log` lines. On Android these
lines reach Logcat under the application's tag, so a coordinate pair is a location
trace that survives in a log the user can share.

The statements:

- `CollectDescriptionCandidates` (`[JNI] checking db box:`) logs the box a coordinate
  is tested against.
- `ResolveRoutePosition` logs the coordinate each `GetClosestRoutableNode` radius
  (1 km, 5 km) resolved at on success.
- `calculateRouteAsync` logs the start and destination coordinate twice each: while
  resolving and on the failure line.

`GeoCoord::GetDisplayText()` and `GeoBox::GetDisplayText()` are the coordinate-pair
formatters. Every one of the seven statements is in the second, legacy route entry;
the first route entry does not carry them.

## Goals / Non-Goals

**Goals:**

- No log line of the JNI bridge carries a coordinate pair.
- The diagnostic value of each line is preserved with precision-free identity: which
  database, which search radius, which route end.
- No behaviour, signature or API change; the change is confined to log lines.

**Non-Goals:**

- A build-time or CI gate that scans the native sources for coordinate log lines; the
  application repository carries that gate for Kotlin only, and the native half belongs
  to a separate change there.
- Any other `GetDisplayText()` use; only the seven statements of the legacy route entry
  are in scope.

## Decisions

**D1 — Replace each coordinate with the identity of the operation.**

Chosen substitutions:

- box check: `[JNI] checking db box of <db->path>` — identity of the database.
- 1 km lookup: `ResolveRoutePosition: found via coord (1 km)`; 5 km:
  `... (5 km)` — the search radius, a distance.
- route end resolving: `calculateRouteAsync: resolving start` / `... dest` — the end.
- failure lines: `... FAILED to find routable node near start` / `... near dest`.

Alternatives considered:

- *Keep the coordinate and surround it with a remark*: does not remove the location
  trace; the point is that no coordinate crosses the log.
- *Log the object reference (file offset, type) the lookup used*: the object path is a
  different code path; these statements are the coordinate fallback and have no object
  to name.
- *Round the coordinate to a coarse precision*: still a position, and the formatter
  the log pipeline reads is the same one being removed.

**D2 — Verify by source, not by a runtime run.**

Chosen: the check is `git grep GetDisplayText` over the file — after the change no
statement of the legacy route entry formats a coordinate. The Java route path cannot be
exercised on this host: the map database under `maps/` is format v26 while the library
reads v27, so there is no usable database and no runtime trace to inspect.

## Risks / Trade-offs

- [A log reader loses the exact coordinate of a failed lookup] → the coordinate was
  already passed to the lookup; a failed route is reported to the caller with the error
  kind (`kRouteFailureStartUnroutable` / `kRouteFailureTargetUnroutable`) and the
  bounded distance probe, so the operator still learns why it failed without the trace.
- [The substitutions are the only content changed, so a future coordinate added next to
  them would not be caught] → acknowledged; a native-source gate is a Non-Goal and a
  candidate for the application repository, noted rather than built here.
