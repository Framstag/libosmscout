# Design

## Context

See `proposal.md` - Why. The relevant current state:

- `libosmscout-client` holds favorite content in `FavoriteLocationService`
  (`libosmscout-client/include/osmscoutclient/FavoriteLocationService.h`). The service is thread-safe per
  call: writers take `std::unique_lock lock(mutex_)`, readers `std::shared_lock`, and `Save()` writes to a
  temporary file and renames it into place. What the service does not have is a notion of replacing its
  whole content: a load is a new instance, a save is `ClearAll()` plus one `AddGroup`/`AddFavorite` per
  entry, and those are separate calls.
- The Java bridge (`libosmscout-client-java/src/OSMScoutClient.cpp`) owned the service as a raw pointer in
  `ClientData`: `loadFavoriteLocations` did `delete data->favService; data->favService = new ...`, and
  `saveFavoriteLocations` did the same plus the rebuild loop from the caller's Java array. `close()` did
  `delete data->favService`.
- Because the pointer was reachable while it was being replaced, two callers could interleave: a reader
  could copy the pointer, a writer could delete it, and the reader then dereferenced freed memory. Even
  without the lifetime problem, a reader between `ClearAll()` and the last `AddFavorite` observed a
  half-rebuilt store.
- The positional move operation added by `client-move-favorite` is a service operation that the new owner
  has to forward, otherwise it becomes unreachable through the bridge.
- The Java suite has no test for the favorite API (`JavaScout/src/test/java/.../client/` has no favorites
  test), so the concurrency and lifetime contract cannot be verified from Java today.
- The `fav-location-java-bindings` capability documented that delegation as "`ClientData` holds a
  `FavoriteLocationService` instance, initialized during `OSMScoutClientBuilder::build()`". The code never
  did that: the pointer was created on the first load or save. The change retires that requirement (see the
  delta spec) rather than carrying a statement forward that was already wrong.

## Goals / Non-Goals

**Goals:**

- Make ownership and replacement of the favorite store explicit and testable in the client library,
  without a Java environment.
- Guarantee three properties with one mechanism: a replacement is atomic for readers, no call runs against
  a destroyed instance, and the "no store" state is reported instead of dereferenced.
- Leave the Java API surface exactly as it is, and shrink the JNI layer instead of adding logic to it.

**Non-Goals:**

- Changing `FavoriteLocationService`: it keeps its per-call thread safety and its file handling, and the
  store does not require a new method from it.
- Adding a Java-level test harness for favorites (none exists; see Open Questions).
- Introducing a rollback of a failed write, or a save-without-replace operation.
- Touching the favorites file format, the type config or any database file.

## Decisions

**D1 - A client-library store component owns the service behind one mutex.**
The component holds the service instance and a `std::mutex`; every operation, including both wholesale
replacements, runs under that mutex.
Alternatives:
- *Atomic `std::shared_ptr<FavoriteLocationService>` swap inside the JNI file*: a replacement becomes one
  pointer store, readers keep the instance alive while they use it, and the rebuild cannot be observed
  half applied because the new instance is complete before the swap. It fails two properties the mutex
  gives: an operation that copies the pointer and then runs can be superseded by a replacement (the
  operation is lost although it reported success), and the "no store" bookkeeping becomes a second piece
  of state to keep consistent with the pointer. It also keeps the ownership logic in the binding, where no
  test can reach it.
- *Expose a replacement operation on the service itself*: the service would have to delete and re-create
  its own content under its own lock, and the caller's "replace the whole content" would become a
  service-level concept with lifetime questions the service cannot answer.
- *Let callers take the service's lock from outside*: the mutex is private for a reason; a public lock
  handle leaks internal synchronisation into every caller and invites lock-order mistakes, and the
  destructive part (`delete`) still needs an owner.
Chosen because a single owner with one lock covers all three goals at once, needs no change to the
service, and makes every JNI function a one-line forward that cannot get the lifetime wrong.

**D2 - The component owns the service through an owning smart pointer.**
The extracted code used a raw owned pointer with `delete service_; service_ = new ...` in three places.
The class is new code and `guidelines/CodeStyles.md` gives raw pointers a non-owning meaning, so the field
becomes `std::unique_ptr<FavoriteLocationService>`: replacement is `service_ = std::make_unique<...>()`,
destruction is `service_.reset()`, and no path can leak or double-delete the instance.
Alternatives:
- *Keep the raw pointer*: byte-identical to the source commit, but it needs manual `delete` in the
  destructor, in the replacement and in the destruction operation, and the class reads as if the pointer
  might not be owned.
- *`std::shared_ptr`*: ownership would be shareable, which is exactly the situation the component removes;
  it would also let a caller keep an instance alive past a replacement without the component knowing.
Chosen because the ownership is exclusive by construction and a `unique_ptr` states it. This is the one
intentional deviation from the source commit, and it is confined to the new file.

**D3 - A failed write keeps the caller's content and reports failure.**
The extracted behaviour rebuilds the service from the caller's snapshot and then persists it; when the
write fails the operation returns false and the in-memory content stays the caller's snapshot.
Alternatives:
- *Roll back to the previous generation on a failed write*: the store would have to keep the old content
  alive across the replacement, which either doubles memory for every save or defers the destruction of
  the old service until the write is known to have succeeded - with the added question of which generation
  is live while the write is in flight.
- *Build the new content first, persist it, and swap only when it succeeded*: the nicest semantics, but the
  service writes its file from the constructor, so it would need a new service-level operation to build
  content without touching the file. That is a change to `FavoriteLocationService` and out of scope here.
Chosen because the caller's snapshot is the intended truth and the failure is reported, so a caller can
retry; the ambiguity about which generation is live is documented in the header and covered by a test
instead of being left implicit.

**D4 - The component lives in `libosmscout-client`, the binding only forwards.**
Alternatives:
- *Keep the ownership logic in the JNI file*: it is the state being replaced. The same defect would have to
  be fixed again in the next binding, and the contract could not be tested without a Java environment.
- *Put the component in the Qt client*: the Qt client shares the fault and would need its own copy.
Chosen because the component is not Java-specific, and the tests (`Tests/src/FavoriteStoreTest.cpp`) then
reach the concurrency contract directly.

**D5 - The component offers an explicit destruction operation in addition to its destructor.**
Alternatives:
- *Destructor only*: `delete data` in the JNI close path would destroy the store anyway, but the release
  point would be implicit, and the reasoning about "a call in flight when the client closes" would live in
  a comment rather than in a testable operation.
- *A separate `Close()` on the client handle*: the store would still need a way to destroy its service, so
  the operation has to exist on the component either way.
Chosen because destruction is then idempotent, testable, and explicit at the place where the client
releases resources.

## Sequence diagram

```
Thread A (load)                        FavoriteStore                        Thread B (read)
     |                                       |                                    |
     | ReplaceByPath(path) ----------------->| lock(mutex_)                       |
     |                                       | delete/assign the new service      |
     |                                       |   (service ctor creates or loads)  |
     |                                       | unlock(mutex_)                     |
     |<----------------- true ----------------|                                    |
     |                                       |<--------- GetGroups() --------------|
     |                                       | lock(mutex_)  <-- waits if replacement is running
     |                                       |   service_->GetGroups()             |
     |                                       | unlock(mutex_)                      |
     |                                       |---------------- complete groups --->|
```

The same shape applies to `ReplaceAndSave(path, groups)`, with the rebuild loop and the write inside the
critical section, so a reader either misses the whole replacement or sees all of it.

## Risks / Trade-offs

- *The store lock is held across the file write* (`ReplaceAndSave` persists inside the critical section) →
  a reader can block for the duration of a write. Mitigation: a write happens once per wholesale
  replacement, the alternative would let a reader observe the store while it is being rebuilt, and the
  service's own `Save()` already releases its internal lock before the rename.
- *A failed write discards the previous generation* (D3) → mitigation: the failure is reported, the
  behaviour is documented on the operation and asserted by a test; a caller that cannot write keeps the
  content it tried to save.
- *Replacing by path reports success even when the file could not be read*: the service constructor
  swallows the load error, so the store can hold an empty store after a failed load. Mitigation: the
  operation is documented as "installs a store backed by the path", and `HasStore()` answers "is there a
  store", not "did the file load". A load-result API would be a change to the service and is not part of
  this change; reported here as a known limitation.
- *Nested locking (store mutex, then the service's own mutex)* → a deadlock would need a path that takes
  the service lock and then the store lock. Mitigation: the store never hands out its service instance and
  never calls back into itself from a service call, so the order is always store-to-service; the rule is
  stated in the header.
- *Thread-sensitive tests*: the concurrency cases depend on a reader thread that is likely to overlap a
  replacement. Mitigation: the completeness check accepts "empty or complete", so a scheduling that
  happens to serialise the threads still passes; the case that must fail when the mutual exclusion is
  removed is the "does not fault / no mixture" one, which was revert-checked when the source commit was
  written.
- *Divergence from the source commit* (the `unique_ptr` of D2) → a later merge of the downstream branch
  will see one changed field and three changed statements in a new file. Mitigation: the deviation is
  documented here and in the commit message, and no other file depends on the field.

## Migration Plan

Plain library update, no data migration: the file format and the Java API are unchanged, so a client built
against the previous version keeps working. Deployment needs no caller change - the JNI functions keep
their names and signatures, and only their body changes. Rollback is a revert of the commits: the store
component disappears, the bridge owns the service pointer again, and favorite files are unaffected.

## Open Questions

- Whether a Java-level test harness for favorites should exist (the Java suite has none, so the "no store"
  and "one-step replacement" behaviour is only verified from C++ today): deferrable, it would not change
  the specs or the approach, and the JNI layer is a one-line forward per function.
- Whether the store should also offer a save-without-replace operation for a caller that mutates the store
  and wants to persist it: deferrable, the current API persists the snapshot it is given.
