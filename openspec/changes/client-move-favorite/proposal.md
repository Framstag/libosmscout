# Proposal

## Why

The Java client API can add, delete and rename a favorite location, but not move it. An
application that lets the user reorder favorites inside a group therefore has to delete the
favorite and add it again, which cannot carry the favorite's attributes: the Java-level add
takes only a name and coordinates, so anything stored in the attribute map is lost in the
round trip. Reordering is a positional operation on data the favorite service already owns,
so the service should offer it instead of pushing a lossy workaround onto every caller.

## What Changes

- The favorite location service gains a positional operation that moves a favorite within its
  group to a target position, given as an index into the list as it looks after the favorite
  has been taken out of its current place. An index outside the list bounds is clamped to the
  first or last position, and moving a favorite to the position it already occupies succeeds
  without changing anything.
- The order of a group's favorites stays the stored order, so a moved favorite keeps its new
  position across a save/reload cycle. No new persisted field is introduced.
- The Java client API exposes the same operation; a negative target index means the first
  position.
- Unknown group and unknown favorite are reported the same way as in the existing mutators
  (false, no change).
- Unit tests cover moves to the front, into the middle and to the end, out-of-range targets,
  a no-op move, unknown group/favorite, and the order surviving a save/reload cycle.

## Capabilities

### New Capabilities
<!-- None: this change only adds an operation to capabilities that already exist. -->

### Modified Capabilities
- `fav-location-service`: adds a positional reorder operation for favs inside a group, with
  bounds clamping and order-persisting behaviour.
- `fav-location-java-bindings`: adds the reorder operation to the set of favorite operations
  the Java client exposes.

## Impact

Affected files and modules:

- `libosmscout-client/include/osmscoutclient/FavoriteLocationService.h` — new public method,
  documented as public API.
- `libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp` — implementation, runs
  under the service's existing write lock.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — new
  native method declaration with Javadoc.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — JNI entry point, delegating to the
  service through the existing `ClientData` handle.
- `Tests/src/FavoriteLocationServiceTest.cpp` — new cases; the file is already built by both
  the CMake and the Meson test descriptions, so no build-system change is needed.

No change to the favorites JSON file format, to the type config or to any database file, so no
`FileFormatVersion.md` version bump applies. No new dependency. No change to `DBThread`,
`MapManager` or the rendering paths. Downstream applications that reorder favorites are the
consumers; no in-repository caller exists yet, so the change is API-only from the repository's
point of view.
