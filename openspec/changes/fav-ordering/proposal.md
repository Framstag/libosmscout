# Proposal

## Why

Users can already reorder the favorites inside a group, but everything around that order is fixed:
groups always appear in name order, a favorite cannot be moved to another group without losing the
data it carries, and starred favorites have no order at all, so a user cannot keep their most
important places in the sequence they want. This makes the favorites list fight the user's mental
model — home belongs at the top and work at the bottom, and a favorite that gets reorganized into
another group should be moved there, not recreated.

## What Changes

- Groups gain an explicit order. The order is chosen by the user, is reported by every group reader,
  and survives a save and reload. A new group is appended at the end. Existing stored files that
  carry no order still load with a defined fallback order and are written back in ordered form on the
  next save, so no user data is lost by upgrading.
- The favorites file stores the version of the format it was written in. Files written before this
  change — no version, groups keyed by name — are still read, so a user's existing favorites survive
  the upgrade, and they are written back in the versioned, ordered form on the next save. A file whose
  version is newer than the client understands is not read and is protected from being overwritten, so
  starting an older client can no longer destroy favorites saved by a newer one. Reading the
  pre-version form is a compatibility path with a planned end: removing it is recorded as a follow-up
  entry rather than done here.
- The position of a group can be changed to a target position, with the same target-position
  semantics the favorites inside a group already use.
- A favorite can be moved from one group to another at a target position. The moved favorite keeps
  its name, coordinates and attributes, including its star. When the destination group already holds
  a favorite of the same name, the move fails and nothing changes.
- Starred favorites gain an order that spans all groups, so the starred places form one sequence the
  user arranges irrespective of which group each star sits in. Star membership keeps its current
  meaning. The starred order can be changed to a target position, and the ordered sequence can be
  read.
- Unstarring a favorite removes it from the starred sequence; starring it again places it at the end
  of that sequence.
- The client store forwards the new operations, the Java client exposes them, and the JavaScout
  favorites dialog lets the user reorder groups, move a favorite to another group, and order the
  starred favorites, reporting a refused move instead of silently ignoring it.

## Capabilities

### New Capabilities
- `fav-group-order`: the user-chosen order of favorite groups, changing a group's position, the
  order a reader observes, and how stored files without an order are handled.
- `fav-star-order`: the cross-group order of starred favorites, changing a star's position in that
  sequence, reading the sequence, and what starring and unstarring do to a position.

### Modified Capabilities
- `fav-location-service`: gains a move of a favorite into another group, including the rule for a
  name collision in the destination group.
- `fav-location-store`: gains forwarding of the group-order, cross-group-move and star-order
  operations.
- `fav-location-java-bindings`: gains the Java surface for the group-order, cross-group-move and
  star-order operations.
- `javascout-fav-location-ui`: gains reordering of groups, moving a favorite to another group, and
  presenting and reordering the starred favorites.

## Impact

Affected files and modules:

- `libosmscout-client/include/osmscoutclient/FavoriteLocationService.h` and
  `libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp` — group ordering, cross-group
  move, star ordering, the load/save handling of the ordered group representation, and the format
  version written on save and checked on load, including the refusal to read or overwrite a file with
  a newer version. The header's public API documentation points at the format document below. Existing
  group lookup and group rename keep working against the ordered collection.
- `Documentation/FavoritesFileFormat.md` — new document describing the persisted favorites file: the
  version field and both known versions, the ordered groups, the favorites and their attributes, the
  star order value, an example document, and the compatibility and removal policy for the pre-version
  form.
- `libosmscout-client/include/osmscoutclient/FavoriteStore.h` and
  `libosmscout-client/src/osmscoutclient/FavoriteStore.cpp` — forwarding of the new operations, and
  preserving a caller-supplied order when the store content is replaced wholesale.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` and
  `libosmscout-client-java/src/OSMScoutClient.cpp` — Java declarations and JNI entry points; the
  ordered starred sequence needs a representation that carries the owning group for each entry.
- `JavaScout/src/main/java/com/framstag/libosmscout/FavLocationDialog.java` — group reordering,
  move-to-group, starred favorites list with reordering, and reporting of a refused move.
- `JavaScout/src/main/java/com/framstag/libosmscout/FavoritePickerDialog.java` — follows group order
  instead of name order.
- `Tests/src/FavoriteLocationServiceTest.cpp` and `Tests/src/FavoriteStoreTest.cpp` — new cases for
  group order, cross-group move (including collision), star order, ordering across save/reload,
  loading a stored file that carries no version and no group order, and a file that carries a newer
  version.
- `TODO.md` — a follow-up entry to remove the pre-version read path once enough releases have passed,
  together with the format version that makes the removal safe to plan.
- `AGENTS.md` — the `Documentation/` entry gains the new format document.

The persisted favorites file changes shape for groups and gains a version field. It is a client data
file, not a database or type-config format, so no `FileFormatVersion.md` version bump applies; the
version described above is the file's own. No new dependency. No change to `DBThread`, `MapManager`,
routing or the rendering paths; the existing favorite-marker rendering reads favorites and is
unaffected by their order.
