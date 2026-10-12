# Design

## Context

See `proposal.md` - Why. Constraining facts, all read from or measured on the current tree:

- The native side keeps one active client process-wide: `ClientData *activeClient` with the comment
  "one active instance at a time, like OSMScoutQt" (`libosmscout-client-java/src/OSMScoutClient.cpp:477-478`).
  `OSMScoutClientBuilder::build()` refuses a second one and returns `null` while one is held
  (`:553-556`). There is no entry point that resets or replaces a stale client.
- Two classes already compensate instead of fixing it: `OSMScoutClientPoiSearchTest.openFixtureClient`
  and `openLookupClient` retry `build()` twenty times with `Thread.sleep(250)` under the comment "The
  JNI allows only one open database client at a time" (`:300-320`, `:370-385`).
- Several classes build a client and release it outside any unwind path, so an aborted assumption or a
  failed assertion leaves it held: `OSMScoutClientGetRoadAtTest:43-60` and `:69-82` release at `:60`/
  `:82` while `Assumptions.assumeTrue(...)` at `:50`/`:76` aborts in between; `OSMScoutClientPoiSearchTest`
  releases three separate clients inside test bodies; `OSMScoutClientInstructionDistanceTest` builds two
  and releases one; `OSMScoutClientNavigationLiveTest` builds five and releases four.
- Twelve test classes use `new OSMScoutClient()` as an availability probe. Two of them
  (`OSMScoutClientNavigationTest`, `OSMScoutClientPoiSearchTest`) then use that probe object as the
  subject of their assertions, and it carries no native handle, so their operations return the "wrong"
  empty result. `OSMScoutClientImportGpxTest` uses the same probe but is unaffected: the JNI function
  ignores its receiver (`jobject /*self*/`, `:1276`) and parses the file itself.
- Operations guard on the handle and silently degrade when it is absent: `startNavigation` logs
  "client not initialised" and returns null (`:7399-7403`), `searchPOIsByTypes` guards on
  `getClientData(env, self)` (`:8122`).
- `JavaScout/src/test/java/com/framstag/libosmscout/DownloadCrashTest.java` is a `main()` reproduction
  tool that happens to live in the test source root.

Measured in this change (`verification.md` holds the numbers):

- **The abandoned client is the whole defect, and it is deterministic.** With a client built and
  abandoned in `OSMScoutClientDataCacheBudgetTest` - the class that runs immediately before
  `OSMScoutClientStyleTest` - all eight `OSMScoutClientStyleTest` tests and all eight
  `OSMScoutClientFavoriteOrderingTest` tests fail at `builder.build()` returning null, in 3 of 3 runs.
  The same injection placed after them in `OSMScoutClientSearchQualityTest` fails only the classes that
  follow it and leaves `OSMScoutClientStyleTest` passing, in 2 of 2 runs. So the leak breaks exactly the
  classes that run after it.
- **A process boundary per test class is not usable here.** `reuseForks=false` gives 7 failing runs in
  10 (against 1 in 10 without it), all of them `OSMScoutClientBasemapConfigTest` expiring a 20 s wait for
  `render()` to become non-null after the basemap lookup directory is cleared. That class fails 2 of 5
  runs even alone in a fresh JVM, and an interleaved 10-run control with the default configuration
  passes 10 of 10 - so the class is the cause, not the environment.
- **The abort that motivated the change did not reproduce** in 30 executions (baseline 10, per-class 10,
  control 10): no exit 134, no `std::bad_alloc`.

## Goals / Non-Goals

**Goals:**

- Remove the class-order dependence by making every test release the client it obtained, on every path
  out of the test - the failure mode measured above, so that no future aborting test can break the
  classes after it.
- Keep the change inside the Java test tree, so the C++ library, the JNI bridge and the shipped client
  behaviour are untouched.
- Leave the suite's coverage unchanged - the tests that were vacuous become meaningful rather than being
  deleted.

**Non-Goals:**

- Changing the single-active-client rule in the client library, or adding a reset/replace entry point.
  It is a deliberate mirror of the Qt client's model, and the measured failure is removed by releasing
  the client, so no API change is warranted.
- Isolating test classes by process (decision 1).
- Fixing the two pre-existing defects the baseline found: the basemap-unload render wait in
  `OSMScoutClientBasemapConfigTest`, and the native library's log output corrupting the runner's
  communication channel. Both are recorded in `TODO.md` for their own changes, per task 5.5.
- Claiming a repeatability the suite does not have: with the basemap defect present, two executions can
  still differ. The spec therefore requires that an execution reports every class it started, and scopes
  the order-independence claim to the isolation defect (decision 6).

## Decisions

### 1. Isolation by releasing the client, not by a process boundary

Every test that obtains a native client releases it on every path out of the test, so the
process-wide slot is free again before the next class starts.

Alternatives considered, with the measurements that decided it:

- **One process per test class** (the approach this design originally chose). It also removes the leak,
  and it makes an abnormally ended class name itself. Rejected on measurement: it turns a 1-in-10 flake
  into 7 failing runs in 10, because `OSMScoutClientBasemapConfigTest` depends on native initialisation
  that an earlier class in a shared process has already paid for, and every class is the first native
  user of its own process. It would trade a defect that needs an aborting test to fire for a defect that
  fires on its own.
- **A reset or replace-stale-client entry point in the client library.** Best ergonomics, and it would
  make a class independent of whatever ran before it even if it did leak. Rejected: it changes
  `libosmscout-client-java`, its public Java API and the `client-*` specs, for a defect that exists only
  in the test suite, and it would weaken the single-active-client rule the shipped clients rely on.
- **Releasing in an unwind path only where a leak was observed.** Rejected: the fault injection shows the
  consequence is total (every later test of every later class fails), and the leak is invisible at the
  moment it happens, so a partial application just moves the surprise.

### 2. How a test obtains and releases its client

Every class that needs a native client obtains it from the builder, asserts it is non-null, and releases
it in a path that runs however the test ends - an `@AfterEach` release for the classes with one client
per test, a `try`/`finally` for the classes that build several. The availability probe stays, but only as
a probe: it builds a client to detect a missing native library and releases it immediately, and is never
handed to an assertion.

Alternatives considered:

- **Delete the probe and rely on the first builder call failing.** Simpler surface, but it turns a
  missing native library into a failed test instead of a skipped one in every class, and the existing
  "Native library not available" text is a useful skip reason.
- **Keep the probe object as the class's client.** The current state; rejected outright, since the probe
  carries no native handle and its operations return empty results that satisfy assertions for the wrong
  reason.
- **A shared base class.** Less repetition, but the multi-client classes do not fit one lifetime, and a
  base class hides which class leaked when the guarantee is broken.

### 3. Making the vacuous tests meaningful instead of deleting them

The tests that assert "no result for an invalid argument" stay and gain the precondition: each
establishes that its client is initialised before asserting. `OSMScoutClientNavigationTest`'s two tests
must reach the invalid-handle path rather than the "client not initialised" path
(`OSMScoutClient.cpp:7399-7403`), and `OSMScoutClientPoiSearchTest`'s three argument-validation tests
must run against an initialised client (`:8122`).

Alternatives considered:

- **Delete them.** They cover argument validation the native layer does enforce, and the coverage would
  be lost; only the assertions were misattributed.
- **Keep them and add new tests with a real client.** Leaves tests whose passing carries no information,
  which is how the defect stayed invisible.

### 4. Disposition of the manual reproduction entry point

`DownloadCrashTest` moves out of the Maven test source root; it is not a test and must not be compiled
into the test classpath or enumerated as a test class.

Alternatives considered: annotating it as disabled (implies it is a test and keeps it in the roster) and
deleting it (it is the only reproduction for a download crash, so deleting it discards evidence).

### 5. The corrupted runner channel stays out of scope

The native library's log output writes to the process output, which the runner records as a corrupted
communication channel. A different runner channel (the `forkNode` parameter, documented by the runner
since 3.0.0-M5, selects sockets instead of process pipes) would remove it.

It was considered for this change - it is one configuration element, and it removes the documented risk
that a corrupted run hangs instead of failing. Rejected to keep the change single-purpose and test-side,
and because the fork-node channel is an additional process whose behaviour should be verified on its
own. Recorded in `TODO.md` (task 5.5).

### 6. The order-independence claim is scoped to the isolation defect

The spec requires that an execution reports every class it started, not that two executions are
identical. The baseline measured 1 differing run in 10 (`OSMScoutClientBasemapConfigTest`'s expired
20 s wait) with the isolation defect not involved at all, so a blanket repeatability requirement would be
a claim this change cannot keep. Order-independence is instead stated as the property the fault
injection verifies: a class's outcome does not depend on the classes that ran before it.

## Sequence

The failure, as measured with the fault injection:

```
 ClassA.test      --build()-->            activeClient = C1
 ClassA.test      --assertion fails-->    test ends; C1 never released
 ClassB.setUp     --build()-->            activeClient != nullptr
                                          OSMScoutClient.cpp:553-556 -> null
 ClassB.test      --assertNotNull-->      FAIL, reason is ClassA        (8 of 8 tests)
 ClassB+1.setUp   --build()-->            null again                    (8 of 8 tests)
```

After the change:

```
 ClassA.test      --build()--> activeClient = C1
 ClassA.test      --assertion fails--> @AfterEach / finally --> C1 closed, slot free
 ClassB.setUp     --build()--> activeClient = C1
 ClassB.test      --...-->                 releases it in turn
```

## Risks / Trade-offs

- [A later test can still abandon a client, and the consequence is total] -> the release is in an unwind
  path in every class that builds one, the fault injection in `verification.md` is the repeatable check
  that the paths work, and the non-null assertion on `build()` names the failure when one is missed.
- [A green run makes the motivating abort look fixed when it is not] -> the verdict comes from ten
  executions against a library built from the current revision, with the execution count and revision
  recorded, and no abnormal end observed in 30 executions so far.
- [The pre-existing basemap defect keeps a rare run red, which could be mistaken for this change's
  outcome] -> it is named in `TODO.md` with its mechanism (`OSMScoutClient.cpp:1523-1524`) and the
  verification note states the measured rate (1 in 10) and the class, rather than presenting the suite
  as deterministic.
- [The corrupted channel remains, and a corrupted run can lose output or hang] -> recorded as an open
  limitation with the named remedy; the isolation guarantee does not depend on it.
- [Removing the retry loops in `OSMScoutClientPoiSearchTest` removes a defence that is currently masking
  something] -> the loops exist because of leaks, the fault injection shows leaks break whole classes
  rather than occasionally failing a build, and the POI tests are run three times consecutively after the
  removal.
- [A class-level release is not enough where one class builds several clients] -> those classes release
  in unwind paths around each client, not per class.

## Migration Plan

Test-side only; no shipped behaviour, no database format, no public API. Rollback is a revert of the
touched test sources. `JavaScout/AGENTS.md` documents the run's requirements and is updated in the same
change so the recipe does not drift from the code.
