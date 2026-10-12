# Proposal

## Why

The JavaScout suite lets a native client, which is process-wide, be owned by test classes that abandon
it: a test that fails or skips after building a client leaves the client active, and every test class
that runs afterwards then fails because the client library refuses to hand out a second one. Measured by
fault injection, one abandoned client fails all sixteen tests of the next two classes, deterministically.
Several classes also assert against a probe client that was never initialised, so they pass for a reason
other than the one they claim.

## What Changes

- A test class owns the native client it uses: it obtains that client and releases it however the test
  ends - normally, with a failed assertion, or with a skipped assumption. A class can no longer leave a
  client behind for whatever runs next.
- Consequence made explicit: a class's outcome does not depend on the classes that ran before it. The
  measured symptom is the one above - an abandoned client fails every test of every later class.
- The guarantee is verified by a fault injection that abandons a client at a known position in the run
  order, not by comparing two run orders, because the class-order symptom only fires when a test aborts
  first and therefore never shows up in a green tree.
- A test asserts the precondition it depends on, so a client that is not initialised can no longer make
  an assertion succeed for a reason other than the one the test states.
- A test source that is not a test - a manual reproduction entry point that the suite currently compiles
  and can pick up - stops being part of the suite.
- An execution reports every test class it started. The suite is not claimed to be repeatable: a
  pre-existing defect in the basemap tests makes one run in ten differ, and it is recorded rather than
  absorbed.
- The abort that motivated this work is re-verified against a library built from the current tree and
  reported as fixed or still open: 30 executions produced no abnormal end, and the recorded evidence for
  it predates the change that addresses the stack it was captured from.
- Explicitly not in this change, and recorded in `TODO.md` for their own changes: the basemap render
  wait just described, and the native library's log output writing to the process output that the test
  runner reports as a corrupted communication channel.

## Capabilities

### New Capabilities

- `javascout-test-run`: an executable JavaScout test run - per-test ownership of the native client, a
  class outcome independent of the classes that ran before it, tests that assert the precondition they
  claim, every started class reported, and a documented verdict on the abort that motivated the change.

### Modified Capabilities

None. No existing capability covers how the JavaScout suite is executed: `javascout-maven-build` is
scoped to `build.sh` jar discovery and build documentation, and the JavaScout CI capabilities cover
building the application, not running its test suite.

## Impact

- `JavaScout/src/test/java/com/framstag/libosmscout/client/` - every class that obtains a native client
  or probes for one: `OSMScoutClientNavigationTest`, `OSMScoutClientPoiSearchTest`,
  `OSMScoutClientGetRoadAtTest`, `OSMScoutClientInstructionDistanceTest`,
  `OSMScoutClientNavigationLiveTest`, `OSMScoutClientSearchQualityTest`,
  `OSMScoutClientAdminRegionScopeTest`, `OSMScoutClientDataCacheBudgetTest`,
  `OSMScoutClientDataCacheSizeTest`, `OSMScoutClientFavoriteOrderingTest`,
  `OSMScoutClientBasemapConfigTest`, `SearchReproTest`.
- `JavaScout/src/test/java/com/framstag/libosmscout/` - `FavLocationDialogOrderingTest` and
  `DownloadCrashTest` (the latter is a manual reproduction entry point, not a test).
- `JavaScout/AGENTS.md` - the test section, so the run's requirements do not drift from the code.
- `TODO.md` - the entry this change closes, and the two pre-existing defects it found and did not fix.
- `JavaScout/pom.xml` is left as it is: a process per test class was measured and rejected (7 failing
  runs in 10 against 1 in 10 without it), and the reason is recorded in the change's design.
