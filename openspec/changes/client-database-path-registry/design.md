# Design

## Context

See `proposal.md` - Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp` holds a `ClientData` for each client. The registered
  map database directories were a plain `std::vector<std::filesystem::path>` member (`knownPaths`):
  `openDatabase(String)` appended to it with a linear scan for duplicates and then called
  `data->dbThread->OnDatabaseListChanged(data->knownPaths)`, which takes the list **by const reference** and
  copies it. `OSMScoutClientBuilder.build()` reset it to an empty vector.
- `DBThread::OnDatabaseListChanged` runs on the database thread and closes and reopens every database of
  the new list, so each call has a database-set change on the cost side.
- Nothing serialises the member: two Java callers on two threads can append at the same time, and the
  caller-side copy inside `OnDatabaseListChanged` reads the vector while another thread reallocates it.
- The Java declaration of the single-directory call already exists; the bridge file also has a known
  pattern of natives without a Java declaration (reported in #1822), so a new native needs its declaration
  as part of the change.

## Goals / Non-Goals

**Goals:**

- Remove the lifetime hazard structurally: no thread should be able to read or copy the registered list
  while another thread mutates it.
- Make "registering a list" one database-set change, and make that property observable in a test without a
  database, a database thread or a Java environment.
- Keep the component free of JNI so it can be tested directly and reused by another binding.

**Non-Goals:**

- Changing `DBThread` or the map manager: the lookup directories stay a separate mechanism.
- Changing the signature or the result of `openDatabase(String)`.
- Coalescing or debouncing publications inside the database thread.
- Adding a Java-level test harness for the bridge (there is no database-path test in the Java suite today).

## Decisions

**D1 - The set lives in a client-library component behind one mutex.**
The new `DatabasePathRegistry` owns the path list and a `std::mutex`; `Register`, `RegisterAll`, `Clear`
and the read operations take the lock, and a snapshot is a value copy made under it.
Alternatives:
- *A mutex in the JNI file around the existing member*: smallest diff, but the contract stays untestable
  without a Java environment and the next binding repeats the same mistake.
- *Copy-on-write `std::shared_ptr<const std::vector<...>>` with an atomic pointer swap*: readers would be
  lock-free, but every registration copies the whole list, so registering K directories one at a time costs
  O(K²) copies, and "is this path already registered" plus the set-change counter would still need
  synchronisation of their own.
- *Publish an immutable list and rebuild it per call*: same copy cost as the previous alternative, with the
  same duplicate-check problem.
Chosen because it removes the hazard with the least machinery, keeps the state in one place, and moves the
contract into a test that needs nothing but the library.

**D2 - The published value is a snapshot, and the lock is released before the database thread is called.**
`openDatabase` and the batch call both call `Snapshot()` (or use the snapshot a registration returned) and
pass that value to `OnDatabaseListChanged`, so the database thread copies and walks its own list.
Alternatives:
- *Pass the registry (or a reference to the list) to the database thread and let it read under the lock*:
  the registry lock would be held across a full close/reopen cycle, including a render-lock wait, and the
  registry would acquire a lock-order relation to the database latch.
- *Return a copy but keep holding the lock while the caller uses it*: the same lock-hold problem with less
  clarity about who releases it.
Chosen because the hazard was precisely a copy taken while another thread mutated the list; handing over a
value removes it regardless of what the database thread does with the list afterwards.

**D3 - One registration call, one set change, with an explicit batch operation.**
`RegisterAll` counts one set change for the whole list.
Alternatives:
- *Let callers loop over the single-path call*: that is the status quo, and it costs one database-set change
  per directory - K closures and reopenings for one logical set.
- *Coalesce publications inside the database thread*: it would hide the cost instead of expressing the
  intent, and it changes when the effect of a registration becomes visible, which callers cannot observe
  from the API.
- *A begin/commit transaction on the registry*: more surface, and a caller that forgets the commit leaves
  the set in a state nothing reports.
Chosen because the caller already holds the list, so asking for it in one call is both natural and exact,
and the resulting count is directly assertable.

**D4 - The batch call answers per input, index-aligned.**
Alternatives:
- *Return the number of newly registered paths*: the caller cannot tell which inputs landed, and duplicate
  inputs make the number ambiguous.
- *Return the complete registered set*: the caller would have to re-derive which of its inputs are in it.
- *Reject a null element with an error*: the repository reports failures as values, and a null entry in a
  Java array is a caller-side mistake that should not stop the rest of the list.
Chosen because an index-aligned boolean array answers the only question the caller has ("is my directory in
the set now?") for every input, including the ones it got wrong.

**D5 - The batch call needs a usable database thread to change anything.**
When the client handle or its database thread is missing, every entry is reported false and the call does
not fault.
Alternatives:
- *Register into the set anyway and publish later*: the caller would see `true` for a directory that no
  database thread will ever open, which is a worse answer than `false`.
- *Throw or return null*: `null` collides with the "empty array" answer, and an exception is not how this
  API reports failure.
Chosen because the Java declaration documents "true when that directory is part of the registered set
afterwards", and a registration that cannot reach the database thread is not part of an acting set.

## Sequence diagram

```
Thread A (openDatabases)                DatabasePathRegistry            Thread B (openDatabase)
     |                                        |                                  |
     | RegisterAll(paths) ------------------->| lock(mutex_)                     |
     |                                        |   skip already registered        |
     |                                        |   append the new ones            |
     |                                        |   snapshot + added count         |
     |                                        | unlock(mutex_)                   |
     |<---- {paths, registered, added} -------|                                  |
     |                                        |<------- Register(path) ----------|
     |                                        |  lock; append; snapshot; unlock  |
     |                                        |------------ true + snapshot ---->|
     | OnDatabaseListChanged(snapshot) ------>|                                  |
     |   (value, lock already released; the database thread copies its own list)  |
     |                                        |                                  |
     | OnDatabaseListChanged(other snapshot) <------------------------------------|
```

Both publications are complete sets taken in registration order, so the database thread never sees a list
that is being mutated, and the later publication carries the earlier one's paths with it.

## Risks / Trade-offs

- *Two publications can overlap on the database thread* (one per call) → both are complete sets and the
  later one supersedes the earlier, but the work is done twice if two calls arrive close together.
  Mitigation: the batch call exists precisely so that a caller with a list publishes once; the database
  thread keeps its own serialisation, which is unchanged.
- *A snapshot is a copy per call* → one vector copy per registration call. Mitigation: one copy per call
  instead of one per directory, and the copy is what makes the publication safe.
- *The set-change counter is diagnostic state that survives `Clear()`* → a caller could misread it as the
  size of the current set. Mitigation: the requirement and the test state that clearing keeps the count and
  empties the set.
- *The batch call reports `false` when the client is unusable, while the caller may believe the directory is
  registered* → Mitigation: the Javadoc and the requirement state it, and the JNI logs a warning; a caller
  that needs certainty reads the returned array.
- *No Java-level test for the bridge* → the JNI part is a short argument-marshalling loop and an
  index-aligned mapping onto the registry result; the set semantics it depends on are covered by the
  registry tests. Recorded as a gap in the tasks and as an open question below.
- *The new component could be misused as a general container* → its name and documentation say it is the
  registry of directories the client was asked to open, and it is only reachable through the client handle
  and the bridge.

## Migration Plan

Additive library and Java API change: `openDatabase(String)` keeps its signature and result, the new call
is new, and no persisted data is involved. A client built against the previous version keeps working; a
caller that wants the batch behaviour calls the new method. Rollback is a revert of the commits: the
bridge holds a plain vector again and the new method disappears, which only affects callers of the new
method.

## Open Questions

- Whether a Java-level test harness for the database-path API should be added (the Java suite has no such
  test today): deferrable, it would not change the specs or the approach.
- Whether the database thread should coalesce publications that arrive in quick succession: deferrable, it
  is a performance question and the current behaviour is already correct.
