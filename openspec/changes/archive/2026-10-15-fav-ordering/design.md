# Design

## Context

See `proposal.md` — Why. Current state that shapes the approach:

- `FavoriteLocationService` keeps groups in `std::map<std::string, FavLocationGroup>`
  (`libosmscout-client/include/osmscoutclient/FavoriteLocationService.h`), so group order is the map's
  key order and cannot be expressed at all. Favorites are a `std::vector<FavLocation>` per group and are
  already reorderable through `MoveFavorite`, which is the convention the new positional operations
  follow.
- Persistence is one JSON document: `groups` as an object keyed by group name, each group holding a
  `favorites` array. `nlohmann::json` objects are sorted by key, so the file cannot carry a group order
  in its present shape; `favorites` is an array and therefore already carries order.
- `FavLocation::attributes` / `FavLocationGroup::attributes` are `std::map<std::string, std::string>`,
  introduced as "extensible attributes for future fields". Two features already use them: `color` on a
  group and `starred` on a favorite.
- `FavoriteStore` owns the service behind one mutex and rebuilds content with `ClearAll()` plus one add
  per group and favorite, so a caller-supplied order is lost today.
- JavaScout is the only in-repository consumer, through `libosmscout-client-java` JNI and
  `FavLocationDialog` / `FavoritePickerDialog`. `favorite-markers` re-renders favorites after a dialog
  save, and reads favorites without depending on their order.

Constraints: the favorites file is user data that must not be lost or silently reordered by an upgrade;
the store's "readers never observe a half-rebuilt store" contract must keep holding; the service stays
thread-safe per call with shared/exclusive locks.

## Goals / Non-Goals

**Goals:**

- A group order that is chosen by the user, observable through every reader, persisted, and changeable
  by a positional operation matching `MoveFavorite` semantics.
- A favorite move into another group that carries the favorite's data, including its star, and refuses
  a name collision without side effects.
- One starred order across groups, manipulated only through positional operations, with membership
  semantics unchanged.
- Loading content written before this change without losing data, and writing it back in the new shape.
- Java and JavaScout exposure of the above, including reporting a refused move.

**Non-Goals:**

- No multi-level group hierarchy; groups stay one level.
- No change to favorite markers, search or routing behaviour; they only follow the new order in lists.
- No new dependency, no database or type-config format change, no `FileFormatVersion.md` bump.
- No reordering UI in the Qt client — it has no favorite caller today.
- No export/import of the favorites file; the ordered shape is not a public interchange format.

## Decisions

### D1: Group order is the stored sequence of an ordered collection, not an attribute

Groups move from the name-keyed map to an ordered collection whose sequence is the order, and the
persisted group container becomes an array of group objects. Lookup by name becomes a scan over that
collection; group counts are small (single-digit to low tens), so the cost is irrelevant next to the
benefit of one order with no derived state. `RenameGroup` loses its `extract`/re-key dance and simply
renames in place, which is what the new spec requires ("renaming keeps the position").

Alternatives considered:

1. *Position number in `FavLocationGroup::attributes`* — keeps the map, the JSON shape and every reader
   untouched, and follows the `color` precedent. Rejected: the order then lives in a user-visible
   attribute that the file format does not treat as structure, so it can be edited into a duplicate or
   contradictory state, and every reader has to sort and invent a tie-break rule. It also makes
   "position" part of the public contract, which `fav-star-order` explicitly forbids for stars.
2. *Keep the map, remember insertion order separately in the service* — rejected: two structures that
   can disagree, and the order still needs a persisted representation, which lands back on option 1 or
   on an array.
3. *Array-shaped groups (chosen)* — one mechanism, mirroring how favorites already work, and the order
   is expressible in the file. Cost: a file-shape change (see D4) and the loss of `map` lookup.

Files: `libosmscout-client/include/osmscoutclient/FavoriteLocationService.h`,
`libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp`.

Risk: `GetGroups()` changes from name order to stored order, so any test or caller that relied on
alphabetical order changes behaviour. Mitigation: no in-repository reader relies on it today
(`FavoriteLocationServiceTest`, `FavoriteStoreTest` assert counts and name lookups), and the specs now
state the order as a contract so the tests can assert it positively.

### D2: Star order is a sparse position attribute per favorite, manipulated by index

A starred favorite carries its place in the starred order in its own attribute map, and the starred
order is the starred favorites sorted by that value. Callers never see the value: they move a star to a
position, and the service places it, spacing values out so a later insert between two entries needs no
renumbering. Unstarring removes the value, so starring again appends at the end.

Alternatives considered:

1. *Root-level ordered array of `{group, name}` references as the source of truth* — reads as a real
   list and makes "append at the end" trivial. Rejected: every group rename, group delete, favorite
   rename and favorite move has to repair references, a missed repair silently loses a star, and the
   existing `fav-star` contract (membership is the `starred` attribute) would have to be superseded.
2. *Hybrid: attribute for membership, root array for sequence* — rejected: same reference-repair cost
   with an extra source of truth to reconcile, and two places can disagree about the same star.
3. *Per-favorite position attribute (chosen)* — no references, so no repair paths; a favorite that is
   moved to another group keeps its star and its place for free; membership stays exactly as `fav-star`
   defines it. Cost: the order is a number in the attributes map, which is why the spec states the value
   is the store's own and not a caller contract, and why the API is positional.
4. *Order the starred favorites by group order and favorite order inside the group* — rejected: gives no
   order the user can choose, which is the point of the requirement.

Files: same as D1, plus the star operations in the store and JNI layers.

Risk: a hand-edited file can contain duplicate or unparsable position values. Mitigation: loading is
tolerant — a missing, unknown or non-numeric value sorts after the known ones and ties fall back to
group order and favorite name, so the reader always produces a total order; the next save writes values
the service owns. This is a defined fallback, not a repair step.

### D3: The cross-group move is one operation in the service, not a composition of delete and add

A new service operation takes source group, favorite name, destination group and target index, performs
the removal and the insertion under the service's existing write lock, and applies the same clamping rule
as `MoveFavorite`. The collision check happens before anything is removed, so a refusal cannot leave the
favorite in limbo. Because the whole thing runs under one lock, no reader can observe the favorite in
neither or both groups.

Alternatives considered:

1. *Compose it from the existing `DeleteFavorite` plus `AddFavorite`* — rejected: the caller would need
   the favorite's full content to re-add it, two calls mean two lock acquisitions and two persistence
   opportunities, and a failure between them loses the favorite. The Java add overload takes only name
   and coordinates, so attributes and the star would be dropped in exactly the way the archived
   `client-move-favorite` change set out to avoid.
2. *Overwrite the destination favorite on collision* — rejected: silently destroys user data, and the
   existing duplicate rules (`AddFavorite`, `RenameFavorite`) refuse instead.
3. *Auto-rename on collision* — rejected: invents a name the user never chose, and the caller cannot tell
   an auto-renamed success from a normal success without inspecting the result.
4. *Fail on collision, one atomic operation (chosen)* — consistent with the existing duplicate rules, no
   data loss, and the dialog can ask the user.

Files: `FavoriteLocationService.{h,cpp}`, `FavoriteStore.{h,cpp}`, `OSMScoutClient.{java,cpp}`,
`FavLocationDialog.java`.

### D4: Loading accepts both group shapes and the first save writes the ordered shape

Loading detects the shape of the persisted group container: an array is read in its order; an object is
read as a name-keyed map and the groups are then ordered by name, which is exactly the order the old
reader reported. Saving always writes the array shape. No data is lost, the fallback order is defined
and testable, and the file converges to one shape on first write.

Alternatives considered:

1. *Refuse files in the old shape* — rejected: breaks every existing user's favorites on upgrade.
2. *Keep writing the old shape when the content came from the old shape* — rejected: two write paths to
   maintain and test, and the order could not be persisted at all for those users, so their arrangement
   would be lost on reload.
3. *Versioned file with a format number* — the versioning itself is decided in D7; the shape of the group
   container is identified by the version rather than guessed from the container type alone.

Migration and rollback: see the Migration Plan section.

### D5: Java exposes an ordered starred entry type instead of parallel arrays

A small class carrying the owning group name and the favorite replaces the need for two parallel arrays,
so the order and the group membership cannot drift apart. The group order needs no new type: the existing
`getFavoriteGroups()` now returns the array in stored order.

Alternatives considered:

1. *Two parallel arrays (groups and favorites)* — rejected: index correspondence is implicit and one
   off-by-one silently pairs a favorite with the wrong group.
2. *Encode the group name into the favorite's name or attributes* — rejected: corrupts user data to carry
   a UI concern.
3. *Dedicated entry class (chosen)* — explicit, and the JNI marshalling stays a straight loop.

Files: `libosmscout-client-java/java/com/framstag/libosmscout/client/StarredFavoriteLocation.java` (new),
`libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`,
`libosmscout-client-java/src/OSMScoutClient.cpp`.

### D6: The dialog reorders with explicit move actions

The dialog gets move-towards-beginning / move-towards-end actions for the selected group and for the
selected starred entry, plus a move-to-group action for the selected favorite. Explicit actions are
keyboard- and test-reachable through the JavaFX control API, which drag-and-drop is not.

Alternatives considered:

1. *Drag-and-drop* — nicer once it works, but needs drop indicators, drag-over validation and a way to
   drive it in a test; deferred, and it can be added later on top of the same service calls.
2. *Numeric position entry* — rejected: exposes the position concept the specs keep private and asks the
   user to do arithmetic.
3. *Move actions (chosen)* — smallest surface that satisfies the UI requirements, and each action maps to
   exactly one service call, so a refused move has one obvious place to be reported.

Files: `JavaScout/src/main/java/com/framstag/libosmscout/FavLocationDialog.java`,
`JavaScout/src/main/java/com/framstag/libosmscout/FavoritePickerDialog.java`.

### D7: The file carries a format version, and a newer version is read as nothing and written by nobody

A version key is written on every save and read on every load. The pre-version form is the lowest known
version and is implied by the absence of the key, so today's files are version 0 and this change's files
are version 1. A file whose version is higher than the highest known version is not parsed as groups at
all: the state reports the version found and that it is unsupported, reads report no groups, and every
operation that would persist over that path fails, leaving the file untouched. The version and its support
state are readable through the service, the store, the Java bindings and the dialog, so a user sees
"written by a newer version" instead of an empty favorites list.

Alternatives considered:

1. *No version key, identify the shape from the container type* (the earlier D4 position) — works for the
   one transition that exists today, but every later change has to infer compatibility from structure, and
   nothing protects a file that a newer client wrote in a shape this client would read as empty.
2. *Version key, best-effort read for unknown versions* — never blocks the user, but a newer file whose
   group container this client misreads becomes "no favorites" and the next save overwrites it, which is
   the data loss the version was meant to prevent.
3. *Version key, refuse to read an unknown version but still allow writes* — the simplest state to build
   (a failed load like a parse error), but the first user action in the dialog can still destroy the newer
   file.
4. *Version key with a read and write block on an unsupported version (chosen)* — the file survives an
   older client being started, at the cost of an explicit unsupported state carried through the service,
   the store, the Java bindings and the dialog. The refusal is on the file path, not on the process: a
   client that opens a different file behaves normally.

Why the version sits inside the file rather than beside it (a `.version` sibling, a lock file): user data
that carries its own version cannot be separated from it by a copy, a backup restore or a sync, and the
favorites file is exactly the kind of file that gets copied by hand.

Files: as in D1, plus the store, the Java bindings and the dialog for the readable state, and
`Documentation/FavoritesFileFormat.md` for the written-down contract that a future reader can compare
against.

Risk: an unsupported file leaves the dialog unusable until the user upgrades or points it elsewhere.
Mitigation: the state names the version found, the file is not damaged, and the client keeps working for
every other file it opens.

### D8: The pre-version read path is planned for removal through a recorded follow-up

Reading version 0 exists so that an upgrade does not lose favorites. It is a compatibility path with a
limited life: once enough releases have shipped that a user's file has been loaded and rewritten in the
versioned form, the version 0 branch can be dropped and a version 0 file can then be handled like any other
unsupported version. The gate to that removal is recorded rather than planned here, because the condition is
time and adoption, not code: the deprecation goes into `TODO.md` as a follow-up so it is not forgotten, and
the version field added by D7 is what makes the removal safe to decide later without guessing.

Alternatives considered:

1. *Remove the pre-version path in this change* — would destroy the favorites of every existing user,
   which is the outcome the compatibility path exists to prevent.
2. *Deprecation window as a runtime timer that disables the read path* — rejected: behaviour that changes on
   a clock is worse than a removal the maintainers decide and test.
3. *Recorded follow-up entry (chosen)* — no code, no timer; the removal is a normal later change against a
   version number it can test.

Files: `TODO.md` (follow-up entry), `libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp`
(the version 0 branch when it is removed).
## Flows

### Cross-group move from the dialog to the file

```
user               FavLocationDialog      OSMScoutClient(JNI)     FavoriteStore     FavoriteLocationService
 |                        |                       |                    |                     |
 | select fav, pick group |                       |                    |                     |
 |----------------------->|                       |                    |                     |
 |                        | moveFavoriteToGroup(src, fav, dst, idx)    |                     |
 |                        |---------------------->|                    |                     |
 |                        |                       | moveFavoriteToGroup|                     |
 |                        |                       |------------------->| lock (store)        |
 |                        |                       |                    |-------------------->| lock (svc)
 |                        |                       |                    |   find dst group    |
 |                        |                       |                    |   name collision?   |
 |                        |                       |                    |     yes -> false,   |
 |                        |                       |                    |            no change|
 |                        |                       |                    |     no  -> move fav |
 |                        |                       |                    |<--------------------| unlock
 |                        |                       |<-------------------| unlock              |
 |                        |  true -> reload lists |                    |                     |
 |                        |  false -> report refusal                   |                     |
 |                        | saveFavoriteLocations(groups)              |                     |
 |                        |---------------------->| ReplaceAndSave     |                     |
 |                        |                       |------------------->| one critical section|
 |                        |                       |                    |  rebuild in order   |
 |                        |                       |                    |-------------------->| Save()
 |                        |<----------------------|                    |<--------------------|
 | refresh markers        |                       |                    |                     |
 |<-----------------------|                       |                    |                     |
```

### Loading a favorites file, by version

```
open file -> parse
              |
              +-- no version key ---------> version 0 (pre-version form)
              |                              groups keyed by name
              |                              -> read groups, order by group name
              |
              +-- version 1 --------------> groups as an ordered array
              |                              -> read groups in array order
              |
              +-- version > known --------> unsupported
              |                              -> report version, no groups
              |                              -> any persist over this path fails
              |
              +-- unparsable -------------> load failure, as today
              |
              v
         in-memory: ordered groups, favorites with star positions,
                    file version + supported flag
              |
        next Save()  (allowed only for a supported version)
              v
         file: { "formatVersion": 1, "groups": [ ... ] }
```

## Risks / Trade-offs

- [An older client destroys a newer client's favorites] → the version field makes that state detectable
  (D7): the version is reported, reads are empty rather than wrong, and every persist over that path fails,
  so the file survives an older client being started. `FileFormatVersion.md` is not applicable because this
  is a client data file, not a database or type-config format.
- [Changing `GetGroups()` semantics silently changes existing readers] → the order is now a spec contract
  with positive assertions in both test files, so a regression is caught rather than tolerated.
- [Star positions leak into the file as an attribute] → stated in `fav-star-order` as store-owned data
  that callers must not set or maintain; all API entry points are positional, so no caller has a reason to
  touch the value.
- [Positional operations with clamping can mask a caller bug (moving to a stale index)] → inherited from
  the existing `MoveFavorite` convention; the specs pin the clamping and no-op semantics so the behaviour
  is deliberate, and a UI that holds a stale selection will move to a defined place, not corrupt data.
- [Hand-edited or duplicated star positions] → load tolerance with a defined sort fallback (D2); the
  reader always yields a total order and the store writes its own values again on save.
- [`FavoriteStore::ReplaceAndSave` rebuild path can reorder groups] → the replacement must append in the
  snapshot's order; the new store requirement has a scenario asserting the supplied order round-trips,
  which fails if the rebuild falls back to sorting.
- [JNI surface grows and can drift from the Java declarations] → each new native method gets a Java
  declaration with Javadoc and a JNI entry point in the same change, and the Java binding spec lists the
  methods and their documented false results.
- [Test churn in `Tests/src/FavoriteLocationServiceTest.cpp` and `Tests/src/FavoriteStoreTest.cpp` because
  group order is now observable] → expected and wanted: the new cases assert order positively, and the
  files are already built by both the CMake and the Meson test descriptions, so no build-system change is
  needed.

## Migration Plan

1. Ship the versioned writer and both readers in the same change: version 0 (no version key, groups keyed by
   name, name-sorted order) and version 1 (this change's shape). Every existing file keeps loading and is
   rewritten as version 1 on the first save.
2. An older client opened on a version 1 file reports the version as unsupported, shows no groups and refuses
   to persist over it (D7), so the new content cannot be destroyed by starting an older build.
3. Record the removal of the version 0 read path in `TODO.md` (D8). The removal is a later change, gated on
   enough releases having shipped that users' files have been rewritten; it then treats a version 0 file like
   any other unsupported version.
4. Rollback of this change is a code revert. A file already rewritten as version 1 keeps its groups and
   favorites but is reported as unsupported by a reverted build, so a user who needs the old build back has
   to restore the file from a backup or have it rewritten by a build that supports version 1.

## Open Questions

- Whether the dialog should additionally support drag-and-drop for reordering (D6). Deferrable: it would
  call the same service operations and change no spec, no approach and no task beyond a later addition.
