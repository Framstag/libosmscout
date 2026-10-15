# Proposal

## Why

A client that is destroyed while one of its asynchronous jobs is still running can kill the whole process. Observed with the Java client: `SearchReproTest` runs two clients in one JVM, the second client's scan dies with `std::bad_alloc` for an allocation of about 140 TB, and the JVM aborts with SIGABRT (exit code 134) even though the machine has free memory and the heap is intact. The allocation comes from a job reading the lookup directory list of a client that has already been destroyed: a worker keeps running after the state its job uses is gone.

Every worker that owns a job queue is affected in the same way, because the queue lives in a base class that is destroyed *after* the members of the class whose jobs use them. Four classes in the client library are in that position, and one of them runs nine such jobs.

## What Changes

- Destroying a worker SHALL stop its job thread and wait for the job in progress before the state that its jobs use is destroyed, so no job can run against destroyed state.
- The order SHALL hold for every worker in the library, including the ones whose jobs run long (a directory scan, a database scan).
- A job that fails SHALL NOT end the process: the failure SHALL be reported through the library's logging, the worker SHALL keep serving its queue, and a caller waiting for that job SHALL be released rather than left waiting forever.
- A job that has not started when the worker is destroyed SHALL NOT run, and its caller SHALL be released; a job that is running SHALL be asked to stop through the `Breaker` it was given, so that the teardown of a long job is bounded instead of waiting for all of it, while a job that ignores the request is still waited for.
- The map database scan SHALL work on the directories that are registered when it starts and SHALL publish the databases it found as one set only when it completes, so that a teardown in the middle of a scan neither publishes a partial set nor reads a list that a concurrent registration is changing.
- The Java client's wait for a completed lookup after a download SHALL be released when that lookup is cancelled, instead of waiting for its timeout.

## Capabilities

### New Capabilities

- `async-worker-lifetime`: how a worker that owns a job queue shuts down, what happens to the work that is in flight or queued when it does, and how a failing job is reported.
- `map-database-scan`: scanning the registered lookup directories for map databases - one complete set per scan, the directories as they were when the scan started, and a scan that stops as part of closing the client that owns it.

### Modified Capabilities

- None.

## Impact

Affected files:

- `libosmscout/include/osmscout/async/AsyncWorker.h`, `libosmscout/src/osmscout/async/AsyncWorker.cpp` — the worker: an explicit shutdown operation that stops the queue and drops the jobs that have not started, asks the jobs in flight to stop, waits for the one that is running, and disposes of the thread; and failure handling for jobs.
- `libosmscout/include/osmscout/async/ProcessingQueue.h` — one operation that stops the queue and drops what it holds under a single lock, so that a job which is returning cannot pick up a queued one.
- `libosmscout-client/include/osmscoutclient/MapManager.h`, `libosmscout-client/src/osmscoutclient/MapManager.cpp` — shuts its worker down before its lookup directories and its lookup mutex go, and its scan works on a snapshot of the registered directories, checks for the stop request per directory and per entry, and publishes only a completed scan.
- `libosmscout-client/src/osmscoutclient/DBThread.cpp` — the same, for the worker with the most jobs (database list changes, style loads, basemap loads, favourite locations, …).
- `libosmscout-client/src/osmscoutclient/MapDownloadService.cpp` — the same, and the workaround that avoided the lookup after a download is removed.
- `libosmscout-client/src/osmscoutclient/POILookupModule.cpp` — shuts its worker down before its state goes; its job reports the POI lookups.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the wait for a completed lookup is released when the lookup is cancelled, and its promise survives a callback that arrives after the call returned.
- `Tests/src/AsyncWorkerTest.cpp` — extended with the shutdown ordering (a job must not observe destroyed members, and a build that gets it wrong is reported by the sanitizer), the discard of a queued job and the release of its caller, a job that observes the stop request, a job submitted after the shutdown, a failing job, and repeated and self-inflicted shutdowns.
- `Tests/src/MapManagerTest.cpp` — new: one complete publication per scan, a database reachable twice published once, an unreadable directory, an empty set, a stopped scan that publishes nothing, a registration during a scan, and repeated teardown while a scan runs.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — the new test registered in both build systems.
- Consumers: every client of `libosmscout-client` (JavaScout, OSMScout2, the Qt client). No API is removed and no behaviour of a successful job changes; a job that would previously kill the process now reports failure.
