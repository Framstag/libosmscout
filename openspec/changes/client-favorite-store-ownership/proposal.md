# Proposal

## Why

Loading or saving the favorites file replaces the whole favorite store: a load builds a fresh service
instance, a save rebuilds the in-memory state from caller-supplied data (clear, then one add per group
and favorite). Those sequences are not atomic in the service, which is only thread-safe per call. A
caller that owns the service pointer therefore cannot replace the store safely: a concurrent reader can
observe a half-rebuilt store, and a concurrent call can run against an instance that has just been
destroyed. The Java bridge is exactly such a caller, and it held the service as an owned raw pointer
that it deleted on load and save.

## What Changes

- A new client-side component owns the favorite service behind one lock and offers each wholesale
  replacement as a single operation: replacing the store with a service backed by a file, and replacing
  the store with caller-supplied content and persisting it.
- Every operation on that component runs under the lock, so a reader sees the store either before or
  after a replacement, never during one, and no call can run against a destroyed instance.
- The component also closes the "no store" case: before a file is set, and after it has been destroyed,
  operations report failure or an empty result instead of dereferencing nothing. Destroying the store is
  explicit and repeatable.
- The component forwards the existing per-favorite operations (groups, favorites, rename, move, starred
  flag, group color) so a caller never needs the service handle to perform a normal operation.
- The Java bridge holds that component by value instead of owning the service pointer, and delegates the
  whole favorite API through it; it no longer deletes or constructs service instances, and its shutdown
  path asks the component to destroy the store rather than deleting a pointer.
- A replaced store stays replaced even when persisting it fails: the failure is reported, the in-memory
  content is the caller's snapshot, and the previous content is not restored.

## Capabilities

### New Capabilities
- `fav-location-store`: owning a favorite store, serialising every access to it, applying a wholesale
  replacement as one atomic step, reporting the "no store loaded" state, and destroying the store
  explicitly.

### Modified Capabilities
- `fav-location-java-bindings`: the native layer no longer owns a favorite service instance; it owns the
  store component and delegates the favorite API through it, and the "no store loaded" state is what the
  Java methods report.

## Impact

Affected files and modules:

- `libosmscout-client/include/osmscoutclient/FavoriteStore.h` — new public class, documented as public
  API.
- `libosmscout-client/src/osmscoutclient/FavoriteStore.cpp` — implementation.
- `libosmscout-client/CMakeLists.txt`, `libosmscout-client/src/meson.build` — register the new files in
  both build systems.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the `ClientData` handle, the favorites JNI
  functions, and the shutdown path.
- `Tests/src/FavoriteStoreTest.cpp` — new test file.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the new test in both build systems.

No change to the favorites JSON file format, to the type config or to any database file, so no
`FileFormatVersion.md` version bump applies. No new dependency. No change to the Java API surface: the
methods, their signatures and their return values stay as they are. Downstream applications that
load or save favorites are the consumers; no in-repository caller exists yet.

This change builds on the positional-move operation of `client-move-favorite`, which the store forwards;
a store that does not forward it would leave the move unreachable through the bridge.
