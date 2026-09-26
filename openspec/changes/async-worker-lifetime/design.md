# Design

## Context

See `proposal.md` for the motivation and `specs/async-worker-lifetime/spec.md` for the contract.

The failure chain, established by investigation (gdb on the reproducing Java test, plus the library sources):

1. `DBThread::Initialize()` submits the directory scan and discards the result: `mapManager->LookupDatabases();` (`libosmscout-client/src/osmscoutclient/DBThread.cpp:170`).
2. `MapManager::LookupDatabases()` posts a job to its worker with `Async<bool>([this](Breaker &) -> bool { ... })` (`MapManager.cpp:38`), which locks `lookupMutex` (a member) and iterates `databaseLookupDirs` (a member) for the whole scan.
3. `OSMScoutClient.close()` frees the client's state, which drops the `MapManager` (`libosmscout-client-java/src/OSMScoutClient.cpp`).
4. `MapManager` derives from `AsyncWorker` (`MapManager.h:39`), so destruction runs `~MapManager()` and destroys its members first, and only then `~AsyncWorker()`, which calls `queue.Stop()` and `thread.join()` (`AsyncWorker.cpp:35-43`). The job in flight therefore runs *after* `lookupMutex`, `databaseLookupDirs` and the rest are gone.
5. Observed with destructor breakpoints, the order is `~MapManager` → `~AsyncWorker`, and the still-running job then asks `operator new` for 140737219920009 bytes: a garbage length taken from the destroyed path list, read by `std::filesystem::path::string()` at `MapManager.cpp:94`.
6. `AsyncWorker::Loop()` calls the job with no exception handling (`AsyncWorker.cpp:53`), so the `std::bad_alloc` leaves the worker thread, `std::terminate` runs and the process aborts with SIGABRT (exit code 134) - the host JVM dies with no `hs_err`, which is why this looked like an environment problem at first.

Not an environment problem: glibc's `MALLOC_CHECK_=3` reports no corruption, all resource limits are unlimited, the process held 266 MB of resident memory with a flat 6.78 GB address space, the machine had ~6 GB available, and the requested size (~2^47) is not a plausible request under any pressure.

The queue's own semantics are "stop accepting, drain what it holds": `ProcessingQueue::Finished()` is documented and implemented as stopped *and* empty (`libosmscout/include/osmscout/async/ProcessingQueue.h:199`), so a job that was already queued when the worker is stopped still runs before the loop exits. Stopping the worker at the start of the derived destructor therefore buys more than the running job: a drained job also runs while the members are still alive, where before it ran after they were gone.

The same shape exists in every `AsyncWorker` subclass whose jobs use its members (`grep`: `MapManager` 2 jobs, `DBThread` 9 jobs, `POILookupModule`, `MapDownloadService`). `DBThread::~DBThread()` even takes a lock on its own `latch` member and closes databases before the worker is stopped, so the same window exists there.

## Goals / Non-Goals

**Goals:**

- Make "destroying a worker" mean "the jobs that use my state are finished with it", for every worker in the library.
- Make a failing job a reported failure instead of a process abort.
- Cover both with deterministic C++ tests that need no database, no map and no renderer.

**Non-Goals:**

- No change to what the jobs do, to their results, or to the queueing semantics of a *successful* job.
- No lifetime redesign (no `shared_from_this`, no ownership change of the workers): shutdown ordering is enough and is what the classes already assume.
- No error channel on `CancelableFuture` (recorded below as the alternative that was rejected for scope).
- Not the other findings from the same investigation (`OSMScoutClientNavigationLiveTest` leaving the process-wide client open when it exits through an assumption).

## Decisions

### D1: An explicit shutdown operation on the worker, called at the start of the derived destructors

Chosen: add `AsyncWorker::Stop()` - stop the queue, then join the worker thread, or detach when called from the worker thread itself, and harmless when called twice - and call it as the first statement of the destructors of the subclasses whose jobs use their members (`MapManager`, `DBThread`, `MapDownloadService`). `~AsyncWorker()` keeps calling it, so a subclass that forgets it merely keeps today's behaviour instead of losing the shutdown.

Alternatives:
1. Rely on the base class destructor alone (the status quo) - rejected: the base is destroyed last by definition, so the derived members are always gone before the worker stops; this is the bug.
2. Keep the queue as a member declared first so it is destroyed last - rejected: members are destroyed before the *base*, so this does not move the shutdown ahead of the derived members; the destruction of the base is already the last step and that is exactly the problem.
3. Have `AsyncWorker`'s constructor/destructor register the derived state so the base can guard it - rejected: it needs a hook in every subclass and a virtual call during destruction, which is not available once the derived destructor has run.
4. Make the jobs hold what they need by value, so they do not touch the worker at all - rejected as the *general* fix: the jobs here also write members (`databaseDirectories`) and emit signals, so it would require restructuring the jobs and their ownership instead of fixing the shutdown order; a job that copies a scan input is a nice-to-have on top, not a substitute.

### D2: A failing job is logged and resolves its future with a default value

Chosen: catch exceptions where a job's promise lives (in `Async<T>`), log them, resolve the promise with a default-constructed result, and add a backstop catch in the worker loop for jobs that are not created through `Async<T>`.

Alternatives:
1. Leave the loop unguarded (the status quo) - rejected: any job failure kills the whole process, including the host JVM; the observable we hit is exactly that.
2. Add an error/failure channel to `CancelableFuture` so a failed job reports an error to its caller - rejected *for this change* because it changes a widely used API (`CancelableFuture<T>::Get()`/`SetValue` and every consumer's expectations) for a case that no in-repository caller currently checks; recorded here as the cleaner follow-up if callers ever need to distinguish "failed" from "returned the default".
3. Catch in the loop and leave the promise unresolved - rejected: the caller's `Get()`/waiting would block forever, which is worse than a default value.
4. Terminate the worker after a failure - rejected: it would turn one failed scan into a dead client for all later jobs.

### D3: `POILookupModule` is left alone, deliberately

Chosen: no change there. It is deleted from inside its own worker thread (`DeleteLater()` stops the queue first, then `Loop()` deletes the object), so at that point nothing else is queued and no job can be in flight besides the deleting one; `~POILookupModule` asserting the thread identity documents that contract. Its shutdown therefore already happens before its members go.

Alternatives:
1. Call `Stop()` there as well for uniformity - rejected: from its own thread `Stop()` takes the detach path (it cannot join itself), which is what the base destructor already does; adding it would only make the contract look weaker than it is.
2. Change `POILookupModule` to be deleted from the owning thread - rejected: it is a different, deliberate ownership model that works for it.

### D4: Deterministic tests in the existing worker test, instead of reproducing the Java abort

Chosen: extend the existing `Tests/src/AsyncWorkerTest.cpp` with a test worker whose member's destructor records "members destroyed" into state owned by the test, and a job that records whether it ran after that - so the ordering is asserted directly and without undefined behaviour - plus cases for a job that was queued, a failing job (the worker survives, a later job runs, the caller is released), shutdown being harmless twice, and self-deletion from the worker's own thread.

Alternatives:
1. Reproduce through `SearchReproTest` and assert the JVM survives - rejected as the *regression test*: the abort needs the timing of a second client against the first one's teardown and was not reproducible with a minimal Java probe, so it would be flaky rather than a contract test; it stays as the manual verification of the fix.
2. Use a sleep-based ordering test - rejected: sleeps are not evidence and would be flaky; the test blocks the job explicitly and releases it only after asserting that the destruction waits.
3. A new test file - rejected: the worker and its queue already have one (`AsyncWorkerTest.cpp`, from the async processing work), the cases belong to the same unit, and extending it leaves both build systems untouched.

## Flows

Destruction before this change (the bug):

```
client.close()                  MapManager (derived)                AsyncWorker (base)
---------------                 --------------------                ------------------
frees ClientData
  drops MapManager ref
                                ~MapManager()  +--- derived members destroyed here
                                (body)         |    databaseLookupDirs, lookupMutex, ...
                                               v
                                               ~AsyncWorker()
                                                 queue.Stop();
                                                 thread.join();   <-- waits for the job...
                                                                     ...which is running
                                                                     against the members
                                                                     destroyed above
                                                                     -> garbage path
                                                                     -> bad_alloc
                                                                     -> no guard in Loop()
                                                                     -> terminate/abort
```

Destruction after this change:

```
                                ~MapManager()
                                  Stop()   <-- first: stops the queue, joins the job
                                    |
                                    +--> the job finishes while every member is still alive
                                         (or is cancelled when it had not started)
                                  derived members destroyed
                                ~AsyncWorker()
                                  Stop()   <-- already stopped: harmless
```

## Risks / Trade-offs

- [A shutdown that waits for a long job makes destruction slower: destroying a client can now block for the duration of a directory scan] → Trade-off accepted and it is what correctness requires; the jobs are short (a scan of a lookup directory, a database list change) and the previous behaviour was to abort the process in the worst case. Where a caller wants to control it, the queue is stopped first, so only the job in flight is waited for.
- [`Stop()` from the worker's own thread detaches the thread instead of joining it, so the thread's resources are released late] → Trade-off accepted: it preserves the existing self-deletion contract (`DeleteLater`/`POILookupModule`) and is the only option that does not deadlock; documented on the method.
- [A failed job now resolves its future with a default value, so a caller that does not check the value may treat a failure as a result] → Mitigation: the failure is logged where it happens and again by the worker loop; the alternative (an error channel on the future) is recorded in D2 for the day a caller needs to distinguish them.
- [Subclasses must remember to call `Stop()`; a future worker may not] → Mitigation: the base destructor still stops and joins (so the only loss is the ordering, i.e. the bug), the requirement and the test spell the contract out, and the two workers with the most jobs are the ones that needed it. A follow-up could assert the ordering in debug builds.
- [The change touches the shutdown path of every client, so a mistake here is visible everywhere] → Mitigation: the shutdown is idempotent, and the full C++ suite plus the Java flow that used to abort are both run before the change is considered done.

## Migration Plan

- Branch off `master`; no data, database-format or API change: `Stop()` is added, nothing removed, and a successful job behaves as before.
- Order: `AsyncWorker` (`Stop()` + failure handling) → the three subclass destructors → the test and its build entries → verification (C++ suite, and the Java test that used to abort).
- Rollback is a revert of the change; the aborting behaviour would come back with it.
