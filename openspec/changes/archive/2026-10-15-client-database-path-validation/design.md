# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h` owns the registered path set behind
  one `std::mutex`, with `Register`, `RegisterAll` (the batch, returning the complete set, the per-input
  disposition and the added count), `Snapshot`, `Size`, `SetChangeCount` and `Clear`. Registration never
  inspects the filesystem.
- `libosmscout-client-java/src/OSMScoutClient.cpp`: `openDatabase(String)` calls `Register` and, when the
  client has a database thread, makes the database thread process the new set; it answers `JNI_FALSE` only
  when the client is unusable. `openDatabases(String[])` registers the list and maps the per-input
  disposition onto an index-aligned boolean array.
- `DBThread::OnDatabaseListChanged` closes and reopens every database of the set it is given, so a
  registration that cannot open anything still costs a full database-set change and a render-lock wait.

## Goals / Non-Goals

**Goals:**

- Make "this path cannot be opened" an answer the caller gets, at the moment of the call that should have
  failed, without changing the batch's tolerance.
- Put the filesystem question in one documented place in the client library, testable without a database
  thread or a Java environment.
- Keep every existing signature and every existing result.

**Non-Goals:**

- Checking whether a directory actually contains a map database (that needs the database thread and is a
  different question from "is this an openable directory").
- Changing the batch result for a path that is not a directory: it stays registered and reported true.
- Changing `DBThread`, the map manager or the lookup-directory scan.
- Coalescing or debouncing publications inside the database thread.

## Decisions

**D1 — The check lives in the client library, next to the registry.**
A documented predicate "is this an openable database directory?" and a validating registration entry point
that uses it.
Alternatives:
- *Check in the JNI bridge only*: the contract would be untestable without a Java environment, and the next
  binding would have to repeat the check.
- *Check inside `Register`/`RegisterAll`*: it would change the batch result for a bad path, which is
  explicitly tolerated today because a directory can vanish between a scan and the call.
- *Let the database thread report an unopenable directory*: it is asynchronous, so the caller's boolean
  could not carry the answer, and it costs the full close/reopen cycle first.
Chosen because the question is about the path, not about the renderer, and only the entry point that may
refuse needs it.

**D2 — The predicate is answered from an error code, not an exception.**
Alternatives:
- *Throwing overload*: the caller sits on the JNI boundary, where an exception would have to be mapped onto
  a Java exception for a case the boolean contract already covers.
- *Querying the path and letting `std::filesystem` throw*: a permission failure or a race with a removal
  would leave the bridge with an exception for an ordinary "not openable" answer.
Chosen because every filesystem failure is the same answer to this question, and the predicate can
therefore be `noexcept`.

**D3 — Only the single-directory entry point validates.**
`RegisterAll` still registers what it was handed, and the bridge reports a non-directory path of a batch by
directory name only.
Alternatives:
- *Validate the batch too, and report a bad element false*: an application that scans a directory and opens
  everything it found would fail the whole batch when one entry vanished in between, and there is no way to
  tell that case from a genuinely wrong path.
- *Validate nothing and only report*: leaves the single-directory call's wrong success in place.
Chosen because the two entry points serve different callers: the single call is addressed by a user, the
batch is a set produced by a scan. The bridge's report about a path of a batch is a report only: it changes
neither the registered set, nor the per-input disposition the batch reports, nor the number of set changes
it counts.

**D4 — A rejected registration publishes nothing.**
The registry counts no set change for a rejected path, so the bridge publishes no set and no database is
closed or reopened.
Alternatives:
- *Register first, publish, then report failure*: the answer and the side effect would disagree, and the
  caller cannot undo the publication.
- *Register and never report*: the status quo, and the reason for the change.
Chosen because "not registered" must also mean "no database-set change", which the existing set-change
count already makes observable.

## Sequence diagram

```
Java caller                 OSMScoutClient (JNI)         DatabasePathRegistry           DBThread
    |                              |                              |                          |
    | openDatabase(path) --------->|                              |                          |
    |                              | RegisterOpenable(path) ----->| is openable? (error code)|
    |                              |                              |   no  -> false, no count |
    |                              |                              |   yes -> Register(path)  |
    |                              |<-- false --------------------|                          |
    |<-- false (warn: name only) --|                              |                          |
    |                              |   (no set is published; every open database stays open)  |
    |                              |                              |                          |
    | openDatabase(path) --------->| RegisterOpenable(path) ----->|   yes -> true + snapshot |
    |                              |<-- true ---------------------|                          |
    |                              | OnDatabaseListChanged(snapshot) ------------------------>|
    |<-- true ---------------------|                              |                          |
```

## Risks / Trade-offs

- *A caller passes a symlink or a mount point and expects a check of the map contents* → the predicate
  answers "is a directory", which is the documented question; map contents are the database thread's to
  judge.
- *The batch and the single call now differ for the same bad path* → deliberate and documented in both
  capability specs and in the Javadoc, and a test pins both directions.
- *A directory disappears between the check and the publication of the single call* → the database thread
  still gets a set containing the path, which is exactly what happens today; the check narrows the window
  and does not close it.
- *The check adds one filesystem call per single-directory open* → it is one `is_directory` on a path the
  caller has just produced, and it happens before the registry lock is taken, so it cannot block another
  opener.

## Migration Plan

Additive: `Register`, `RegisterAll` and both Java signatures are unchanged; `openDatabase` reports failure
for a case that previously reported success, which is the defect being fixed, and no persisted data is
involved. Rollback is a revert of the commits, which leaves the predicate and the validating entry point
unused.

## Open Questions

- Whether the batch should report a non-directory input as false behind a flag: deferrable, no current
  caller wants to fail a scanned batch.
- Whether an unopenable directory should be retried when it appears later: deferrable, the caller can call
  the single-directory entry point again.
