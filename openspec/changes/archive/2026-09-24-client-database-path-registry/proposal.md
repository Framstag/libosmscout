# Proposal

## Why

Two code paths in one process register map database directories: an application's map start and, in the
downstream car client, a session warmup. They run on different threads, and the registered path list was
plain data in the client handle: the bridge appended to it under no lock while the database thread copied
the same list on the calling thread, so an append could reallocate the list while the database thread was
reading it - a use-after-free that takes the process down.

The same list has a second cost. Every single registration asks the database thread to close and reopen
every open database, so registering K directories one by one costs K complete database-set changes for one
logical set, each of them with a render-lock wait. A caller that already holds the list has no way to hand
it over in one step.

## What Changes

- The client library owns the set of registered map database directories behind one lock, as a component
  that can be used and tested without the Java bridge. Registering a single directory and registering a
  list are each one operation.
- Every operation returns the complete set as a value snapshot taken under that lock, and the set is only
  ever published from such a snapshot, so a reader can no longer copy a list that another thread is
  mutating.
- Registering a list is one set change, independent of the number of directories in it, and the operation
  reports the disposition of every input: which directories are part of the registered set afterwards and
  how many this call added.
- The Java client can hand over a whole list of directories in one call and receives an index-aligned
  answer for every input. A single-directory call registers through the same set, so both entry points
  share one state.
- The existing single-directory Java call keeps its signature and its result; only its internal handling
  of the path set changes.

## Capabilities

### New Capabilities
- `database-path-registry`: owning the set of registered map database directories, registering one or many
  paths idempotently, publishing the complete set as a value snapshot, and counting the set changes so
  that "one registration call, one database-set change" is observable.
- `client-java-database-paths`: the Java client's way of opening a whole list of map database directories
  in one call, and the per-input result it gets back.

### Modified Capabilities
<!-- None: no existing capability's requirements change. The single-directory Java call keeps its
     signature and its result, so its documented behaviour is unaffected. -->

## Impact

Affected files and modules:

- `libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h` — new public component, documented as
  public API.
- `libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp` — implementation.
- `libosmscout-client/CMakeLists.txt`, `libosmscout-client/src/meson.build` — register the new files in
  both build systems.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the client handle holds the registry instead of a
  path vector, the single-directory open registers through it and publishes a snapshot, and a new JNI
  entry point registers a whole list.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the declaration and
  documentation of the new call.
- `Tests/src/DatabaseOpenTest.cpp` — new test file.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the new test in both build systems.

No change to the favorites or map data formats, to the type config, or to any database file, so no
`FileFormatVersion.md` version bump applies. No new dependency. `openDatabase(String)` keeps its signature
and its result. The map lookup directories are unaffected: they are scanned by the map manager and are not
part of the registered set.

This change is independent of the store ownership change (`client-favorite-store-ownership`); it touches
the same bridge file but a different part of the client handle.
