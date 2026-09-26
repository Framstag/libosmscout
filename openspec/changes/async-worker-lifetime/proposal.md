# Proposal

## Why

A client that is destroyed while one of its asynchronous jobs is still running can kill the whole process. Observed with the Java client: `SearchReproTest` runs two clients in one JVM, the second client's scan dies with `std::bad_alloc` for an allocation of about 140 TB, and the JVM aborts with SIGABRT (exit code 134) even though the machine has free memory and the heap is intact. The allocation comes from a job reading the lookup directory list of a client that has already been destroyed: a worker keeps running after the state its job uses is gone.

Every worker that owns a job queue is affected in the same way, because the queue lives in a base class that is destroyed *after* the members of the class whose jobs use them. Four classes in the client library are in that position, and one of them runs nine such jobs.

## What Changes

- Destroying a worker SHALL stop its job thread and wait for the job in progress before the state that its jobs use is destroyed, so no job can run against destroyed state.
- The order SHALL hold for every worker in the library, including the ones whose jobs run long (a directory scan, a database scan).
- A job that fails SHALL NOT end the process: the failure SHALL be reported through the library's logging, the worker SHALL keep serving its queue, and a caller waiting for that job SHALL be released rather than left waiting forever.
- The behaviour of jobs that are queued but not yet started when the worker is destroyed SHALL stay as it is today: they are cancelled by stopping the queue.

## Capabilities

### New Capabilities

- `async-worker-lifetime`: how a worker that owns a job queue shuts down, and what happens to the work that is in flight or queued when it does, plus how a failing job is reported.

### Modified Capabilities

- None.

## Impact

Affected files:

- `libosmscout/include/osmscout/async/AsyncWorker.h`, `libosmscout/src/osmscout/async/AsyncWorker.cpp` — the worker: an explicit shutdown operation, and failure handling for jobs.
- `libosmscout-client/include/osmscoutclient/MapManager.h`, `libosmscout-client/src/osmscoutclient/MapManager.cpp` — shuts its worker down before its lookup directories and its lookup mutex go.
- `libosmscout-client/src/osmscoutclient/DBThread.cpp` — the same, for the worker with the most jobs (database list changes, style loads, basemap loads, favourite locations, …).
- `libosmscout-client/src/osmscoutclient/MapDownloadService.cpp` — the same.
- `libosmscout-client/src/osmscoutclient/POILookupModule.cpp` — reviewed; it is deleted from its own worker thread after its queue has been stopped, so no change is needed there (recorded in the design).
- `Tests/src/AsyncWorkerTest.cpp` — extended with the shutdown ordering (a job must not observe destroyed members), a drained queued job, a failing job, and repeated and self-inflicted shutdowns. No build-system change: the test exists in both build systems already.
- Consumers: every client of `libosmscout-client` (JavaScout, OSMScout2, the Qt client). No API is removed and no behaviour of a successful job changes; a job that would previously kill the process now reports failure.
