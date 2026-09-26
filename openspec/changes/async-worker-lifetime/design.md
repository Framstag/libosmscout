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

Findings while implementing the extension of this change (the parts that make the shutdown bounded and the scan safe):

- `ProcessingQueue::Stop()` drains *by design* and that semantics is used elsewhere: `Worker`/`Pipe`/`Consumer` users and their tests (`Tests/src/WorkQueueTest.cpp`, `Tests/src/LatchTest.cpp`, `Tests/src/AsyncProcessingTest.cpp`) rely on a stopped queue still handing out what it holds. Dropping the pending jobs therefore needs its own operation, and it has to drop them *under the same lock* that stops the queue.
- Stopping and dropping must happen *before* the jobs in flight are asked to stop. With the reverse order, a job that observes the request returns immediately, and the worker loop - which is not stopped yet - can pick up a queued job in that window. The Meson configuration of `AsyncWorkerTest` reproduced exactly that interleaving; the CMake configuration happened not to. This is why `StopAndDiscard()` is one lock acquisition and is called first.
- `POILookupModule::~POILookupModule()` asserts that it runs on its own worker thread, and that assertion is what `GetThreadId()` is read for. A shutdown that detached the thread early would leave `GetThreadId()` empty for that destructor, so a self-inflicted stop does not dispose of the thread; the destructor does.

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

Chosen: add `AsyncWorker::Stop()` - it marks the worker as stopped, stops the queue and drops the jobs that have not started in one step, asks the jobs that are in flight to stop through the `Breaker` they were given, and then waits for the job that is running; it is harmless when called twice. It is called as the first statement of the destructors of the subclasses whose jobs use their members (`MapManager`, `DBThread`, `MapDownloadService`, `POILookupModule`). `~AsyncWorker()` calls it as well, so a subclass that forgets it keeps at least the old behaviour instead of losing the shutdown, and disposes of the worker thread that a self-inflicted stop could not join.

Jobs register the `Breaker` they receive with the worker when they are submitted, so the shutdown can reach the job that is running as well as the ones that are queued (whose breakers are dropped, which cancels their futures). A job that polls the breaker stops at its next check; the scan does so per directory and per entry. A job that does not poll is still waited for - correctness does not depend on cooperation, only the latency of the teardown does.

Alternatives:
1. Rely on the base class destructor alone (the status quo) - rejected: the base is destroyed last by definition, so the derived members are always gone before the worker stops; this is the bug.
2. Keep the queue as a member declared first so it is destroyed last - rejected: members are destroyed before the *base*, so this does not move the shutdown ahead of the derived members; the destruction of the base is already the last step and that is exactly the problem.
3. Have `AsyncWorker`'s constructor/destructor register the derived state so the base can guard it - rejected: it needs a hook in every subclass and a virtual call during destruction, which is not available once the derived destructor has run.
4. Make the jobs hold what they need by value, so they do not touch the worker at all - rejected as the *general* fix: the jobs here also write members (`databaseDirectories`) and emit signals, so it would require restructuring the jobs and their ownership instead of fixing the shutdown order. Kept as an addition where it is cheap: the scan copies the registered directory list into the job.
5. Keep the drain semantics and only wait for everything (the first commit of this change) - rejected after the extension: a queue of long jobs then delays the teardown by the sum of their run times, and the queued jobs run while the owner is already tearing down. Dropping them and releasing their callers is both bounded and closer to what the proposal always said ("cancelled by stopping the queue"; the first version of the spec contradicted it).
6. Ask the jobs to stop first, then stop and drop the queue - rejected: it opens a window in which a job that returns on request lets the worker pick up a queued job (reproduced under Meson, see Context).

### D2: A failing job is logged and resolves its future with a default value

Chosen: catch exceptions where a job's promise lives (in `Async<T>`), log them, resolve the promise with a default-constructed result, and add a backstop catch in the worker loop for jobs that are not created through `Async<T>`.

Alternatives:
1. Leave the loop unguarded (the status quo) - rejected: any job failure kills the whole process, including the host JVM; the observable we hit is exactly that.
2. Add an error/failure channel to `CancelableFuture` so a failed job reports an error to its caller - rejected *for this change* because it changes a widely used API (`CancelableFuture<T>::Get()`/`SetValue` and every consumer's expectations) for a case that no in-repository caller currently checks; recorded here as the cleaner follow-up if callers ever need to distinguish "failed" from "returned the default".
3. Catch in the loop and leave the promise unresolved - rejected: the caller's `Get()`/waiting would block forever, which is worse than a default value.
4. Terminate the worker after a failure - rejected: it would turn one failed scan into a dead client for all later jobs.

### D3: `POILookupModule` stops its worker, and a self-inflicted stop does not dispose of the thread

Chosen: `~POILookupModule()` calls `Stop()` before it asserts the thread identity. In its normal lifecycle it is deleted from inside its own worker thread (`DeleteLater()`), where `Stop()` cannot join and therefore does not: the destructor asserts the identity, the thread ends after the queue has been drained, and `~AsyncWorker()` detaches it. When the object is destroyed from another thread (a path the assertion only guards in builds with assertions enabled), `Stop()` now waits for a running lookup before the state it reads is released.

Alternatives:
1. Leave it alone (as the first version of this design did, relying on the assertion) - rejected: release builds have the assertion compiled out, and then a lookup could run against destroyed state exactly like the scan did; the cost of calling `Stop()` is zero in the lifecycle it documents.
2. Detach the thread inside `Stop()` when it is called from the worker itself, i.e. dispose of it early - rejected: `GetThreadId()` returns an empty id for a detached thread, so the assertion in that very destructor would fail; the destructor disposes of the thread instead.

### D4: Deterministic tests in the existing worker test, instead of reproducing the Java abort

Chosen: extend the existing `Tests/src/AsyncWorkerTest.cpp` with a test worker whose member's destructor records "members destroyed" into state owned by the test, and a job that records whether it ran after that - so the ordering is asserted directly and without undefined behaviour - plus cases for a job that was queued, a failing job (the worker survives, a later job runs, the caller is released), shutdown being harmless twice, and self-deletion from the worker's own thread.

Alternatives:
1. Reproduce through `SearchReproTest` and assert the JVM survives - rejected as the *regression test*: the abort needs the timing of a second client against the first one's teardown and was not reproducible with a minimal Java probe, so it would be flaky rather than a contract test; it stays as the manual verification of the fix.
2. Use a sleep-based ordering test - rejected: sleeps are not evidence and would be flaky; the test blocks the job explicitly and releases it only after asserting that the destruction waits.
3. A new test file - rejected: the worker and its queue already have one (`AsyncWorkerTest.cpp`, from the async processing work), the cases belong to the same unit, and extending it leaves both build systems untouched.
4. Assert the ordering only through observable state owned by the test (the `MemberGuard` cases above) - kept for the ordering itself, but not sufficient alone: those cases read valid memory by construction, so a build that gets the ordering wrong still passes them silently. The `LateJobWorker` case therefore reproduces the access against the members' heap buffers, which a sanitizer build reports as a use-after-free, and the scan has its own file (`MapManagerTest.cpp`, registered in both build systems).

### D5: The scan works on a snapshot and publishes only a completed scan

Chosen: `MapManager::LookupDatabases()` copies the registered directory list under `lookupMutex`, walks the copy, checks the `Breaker` before each directory and each entry, and only assigns `databaseDirectories` and emits `databaseListChanged` when the scan completed. A scan that is stopped publishes nothing and leaves the previously published set untouched.

Alternatives:
1. Keep reading the member list and rely on the lock (the status quo) - rejected: the lock protects the object only while it lives, and destruction does not take it; this is the crash that motivated the change.
2. Take the snapshot on the calling thread before submitting the job - equivalent in effect, but it copies for every registration that a later one supersedes, and it does not protect the publication.
3. Check the stop request only between directories - rejected: a single directory with thousands of entries would still delay the teardown for the whole walk, and the check costs an atomic load next to the `stat` the walk already performs.

### D6: The Java wait for a lookup is released by cancellation

Chosen: the bridge registers both an `OnComplete` and an `OnCancel` callback for the lookup it waits for and keeps the promise alive until one of them has run, so a lookup that is cancelled during teardown releases the wait immediately instead of reaching the 30 s timeout - and a callback that arrives after the call returned cannot touch a dead stack object.

Alternatives:
1. Leave the wait as it is - rejected: a teardown would stall the JNI call for up to 30 s, and with the new cancellation the completion callback no longer fires for a stopped scan.
2. Wait on `StdFuture()` and let the cancel exception resolve it - works, but replaces a timeout-bounded wait with an exception in a JNI entry point.

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
                                  Stop()   <-- first
                                    |
                                    +--> queue stops and drops the jobs that have not started
                                    |    (their callers are released: cancelled)
                                    +--> the job in flight is asked to stop (its breaker);
                                    |    a polling job ends at its next check
                                    +--> the job that is running is waited for, while every
                                         member is still alive
                                  derived members destroyed
                                ~AsyncWorker()
                                  Stop()   <-- already stopped: harmless
                                  thread disposed of when Shutdown could not join it
```

The scan, in the same wording:

```
  lookup job            worker (MapManager)                owner thread
  ----------            ------------------                 ------------
  lock(lookupMutex)
  snapshot = registered directories (value)
  for directory, for entry:
    breaker aborted?  -> stop, publish nothing, return
  publish the found set, emit databaseListChanged
                        <-- (destroy) Stop(): queue stopped+dropped,
                            breaker broken -> the walk ends at its next
                            entry check, no publication
```

## Risks / Trade-offs

- [A shutdown that waits for a long job makes destruction slower: destroying a client can now block for the duration of a directory scan] → Mitigated rather than accepted: jobs poll the `Breaker` they receive, the scan checks it per directory and per entry, and the queued jobs are dropped, so only the job in flight is waited for and only until its next check. A job that ignores the breaker still delays the teardown, which is the correctness half of the trade-off.
- [A queued job no longer runs at shutdown, so a caller that relied on the drain semantics sees a cancelled future instead of a result] → Trade-off accepted and tested: the futures of dropped jobs are cancelled (their callers are released, `IsCanceled()` is true, and a blocking `StdFuture().get()` throws "Canceled" instead of hanging forever); the only in-repository caller that waits is the Java bridge, which now handles cancellation (D6). No production caller depended on a queued job running during destruction, and the two tests that asserted the drain (one per build system's suite) were written to the new contract.
- [A job that does not poll its breaker is waited for, so a teardown can still be slow] → Accepted: the alternative is abandoning the job while it points into state that is being released, which is the bug.
- [A stop that drops jobs makes a "fire and forget" submission silently do nothing] → Mitigated: the submission returns a future that reports cancellation, `Async()` refuses to queue a job once the worker has stopped, and the worker logs when it is destroyed without having been stopped while a job was in flight.
- [`Stop()` from the worker's own thread detaches the thread instead of joining it, so the thread's resources are released late] → Trade-off accepted: it preserves the existing self-deletion contract (`DeleteLater`/`POILookupModule`) and is the only option that does not deadlock; documented on the method.
- [A failed job now resolves its future with a default value, so a caller that does not check the value may treat a failure as a result] → Mitigation: the failure is logged where it happens and again by the worker loop; the alternative (an error channel on the future) is recorded in D2 for the day a caller needs to distinguish them.
- [Subclasses must remember to call `Stop()`; a future worker may not] → Mitigation: the base destructor still stops and joins (so the only loss is the ordering, i.e. the bug), the requirement and the test spell the contract out, and the two workers with the most jobs are the ones that needed it. A follow-up could assert the ordering in debug builds.
- [The change touches the shutdown path of every client, so a mistake here is visible everywhere] → Mitigation: the shutdown is idempotent, and the full C++ suite plus the Java flow that used to abort are both run before the change is considered done.

## Migration Plan

- Branch off `master`; no data, database-format or API change: `Stop()` is added, `ProcessingQueue` gains one operation, nothing is removed, and a successful job behaves as before.
- Order: `AsyncWorker` (`Stop()` + breaker registration + failure handling) → `ProcessingQueue` (stop and drop in one step) → the four subclass destructors → the scan snapshot and its stop checks → the Java bridge wait → the tests and their build entries → verification (both C++ suites, the sanitizer suite, and the Java reproduction that used to abort).
- Rollback is a revert of the change; the aborting behaviour would come back with it.
