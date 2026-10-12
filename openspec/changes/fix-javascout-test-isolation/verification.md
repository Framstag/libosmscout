# Verification

Evidence for `fix-javascout-test-isolation`. Library revision for every run in this note unless
stated otherwise: `71a2cc20978dc5e8f7567d9a299f93273a110992` (branch `remove-dead-maptilecache`).

Sections are named by subject, not by task id: the per-class process under "2. Per-class isolation
configuration" was a task that the change removed after this measurement, so its number no longer
exists in `tasks.md`.

## 1. Baseline and re-verification of the motivating abort

### 1.1 Native client library built from the current revision

```
$ meson compile -C build-meson libosmscoutclientjava osmscout_client_java
[68/68] Linking target libosmscout-client-java/src/libosmscout_client_java.so.1.1.1
real 2m17.458s
```

Target names differ from the change's task text, which names the two output paths
(`libosmscout-client-java/java/libosmscoutclientjava.jar`,
`libosmscout-client-java/src/libosmscout_client_java.so.1.1.1`); meson resolves meson target names, so
`meson compile` rejects the paths with `target not found`. The equivalent target names are
`libosmscoutclientjava` and `osmscout_client_java`.

Artifacts rebuilt from revision `71a2cc209`:

| artifact | timestamp |
|---|---|
| `build-meson/libosmscout-client-java/java/libosmscoutclientjava.jar` | 2026-09-28 20:26 |
| `build-meson/libosmscout-client-java/src/libosmscout_client_java.so.1.1.1` | 2026-09-28 20:27 |

No compile warning was introduced by this step (the tree's pre-existing warnings are unrelated to this
change, which touches no C++ source).

### 1.2 Pre-change baseline

Ten consecutive executions of `JavaScout/test.sh build-meson` (which runs `mvn test` against the
library built in 1.1):

| run | wall | result |
|---|---|---|
| 1 | 16 s | 232 run, 0 failures, 0 errors, 20 skipped - success |
| 2 | 41 s | **232 run, 2 failures**, 0 errors, 20 skipped - failure |
| 3 | 11 s | 232 run, 0 failures, success |
| 4 | 12 s | 232 run, 0 failures, success |
| 5 | 11 s | 232 run, 0 failures, success |
| 6 | 12 s | 232 run, 0 failures, success |
| 7 | 13 s | 232 run, 0 failures, success |
| 8 | 11 s | 232 run, 0 failures, success |
| 9 | 11 s | 232 run, 0 failures, success |
| 10 | 11 s | 232 run, 0 failures, success |

Flake rate: 1 of 10. The failures in run 2:

```
OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads:357
  the map must keep rendering after the basemap is unloaded ==> expected: not <null>
  Time elapsed: 20.24 s
OSMScoutClientStyleTest.testLoadStyleSheetUnloadableKeepsPreviousStyle:193
  switching to an unloadable stylesheet must fail once the DB is open ==> expected: <true> but was: <false>
  Time elapsed: 10.13 s
```

Both are waits that consumed their whole budget (`renderFixture` polls for 20 s,
`testLoadStyleSheetUnloadableKeepsPreviousStyle` polls 50 x 200 ms), which is where run 2's 41 s
wall-clock time comes from - that run was not slow, two waits expired.

What the baseline does **not** show, and what matters for the change's scope:

- **No abnormal end in any of the ten executions.** No `terminated without properly saying goodbye`,
  no exit 134, no `std::bad_alloc`. The abort that motivated the change did not reproduce against a
  library built from the current revision in ten runs (the verdict is recorded in 6.4).
- **The two failures are not preceded by a visible difference.** A diff of the per-class result lines
  of run 1 (passing) against run 2 (failing) shows only the two failures; every other class reports
  the same counts and the same skips, so no class skipped a test in one run and not the other.
- **The native scan volume is identical in passing and failing runs.** Every run produces one new
  `*-jvmRun1.dumpstream` with 43 `Lookup databases` records and 103-110 `Corrupted channel` records;
  the passing runs 1, 3, 4, 5 and the failing run 2 all report exactly 43 scans. Extra native churn
  therefore does not discriminate the failing run.
- **Neither failing class leaks a client.** `OSMScoutClientBasemapConfigTest.build()` assigns the field
  `client` (`:145`), so its `@AfterEach closeClient()` covers every client it builds, and the failing
  test's own `build()` call succeeded (`assertNotNull(client, "builder.build() must create a client")`
  did not fire). `OSMScoutClientStyleTest.rebuild()` closes its previous client before building, and
  its `assertNotNull(client, "builder.build() must succeed after closing the previous client")` did not
  fire either. So the two failures are not the leaked-client signature this change fixes.

Consequence for the plan: this baseline is the reference for task 2.2. If the per-class process in 2.1
does not remove these two failures, that is the measured outcome design.md's Non-Goals anticipated and
it is reported rather than absorbed (see 2.2/6.4).

## 2. Per-class isolation configuration

### 2.1 Per-class process - measured, reverted

The configuration was applied (`<reuseForks>false</reuseForks>` in the surefire configuration of
`JavaScout/pom.xml`) and measured against the 1.2 baseline, then reverted because it fails task 2.2's
verification. `JavaScout/pom.xml` is byte-identical to its pre-change state
(blake3 `0740aeaccbbf2944098e7db67d46d74f0ec917537c6b09be5938ad559971f1cc`).

Ten executions with the per-class process:

| run | wall | result |
|---|---|---|
| 1 | 47 s | 1 failure |
| 2 | 51 s | 1 failure |
| 3 | 27 s | pass |
| 4 | 45 s | 1 failure |
| 5 | 50 s | 1 failure |
| 6 | 44 s | 1 failure |
| 7 | 46 s | 1 failure |
| 8 | 29 s | pass |
| 9 | 47 s | 1 failure |
| 10 | 22 s | pass |

Flake rate: **7 of 10**, against 1 of 10 for the baseline. Every failure is in the same class:

```
OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads:357
  the map must keep rendering after the basemap is unloaded ==> expected: not <null>       (20.2 s) x6
OSMScoutClientBasemapConfigTest.testBasemapLookupDirectoryWithoutDatabaseKeepsRendering:373
  a directory without an openable basemap must not stop map rendering ==> expected: not <null> (20.1 s) x1
```

The configuration is in effect: the suite's wall-clock time rises from 10-16 s to 22-51 s, consistent
with one JVM start per test class plus the 20 s waits that expire in the failing runs.

Interleaved control, run immediately afterwards with `reuseForks` back at its default: **10 of 10 pass**.
The environment therefore does not explain the difference.

Single-class control, `mvn test -Dtest=OSMScoutClientBasemapConfigTest` in a fresh JVM with no other
class before it: **2 of 5 runs failed**, and in a second loop 1 of 6. The class fails as the only native
user of its process, which is what a per-class process makes of every class.

Mechanism: the render entry point returns no pixels when the database thread reports neither a database
nor a basemap,

```
libosmscout-client-java/src/OSMScoutClient.cpp:1523-1524
  if (databases.empty() && !basemapDatabase) {
    return;
  }
```

and the two failing tests wait for `render()` to become non-null after the basemap was unloaded
(`renderFixture` polls for 20 s). In a failing run the class's dumpstream contains only
`Lookup databases` and `Created new style` lines - no `[JNI] render:` warning - so no logging guard was
reached: the render returned at the silent early return above. In a cold process the post-unload state
is reached inside the 20 s window far less often than in the shared process, where an earlier class has
already paid the one-time native initialisation.

Consequences, all measured rather than assumed:

- Task 2.1's mechanism does not satisfy task 2.2's verification. It widens an existing rare flake into a
  frequent one, so it must not land as specified.
- **The symptom this change was aimed at did not reproduce.** In 20 executions with the default shared
  process (baseline 10 + control 10) the only failure is run 2's two expired polls. No execution failed
  with `builder.build() must create a client` / `must succeed after closing the previous client`, i.e.
  the "an earlier class left the client behind" symptom was never observed - the leaks the design found
  in the sources are real in code but did not fire in these 20 runs, because firing them needs a test
  that aborts first.
- **No abnormal end in 30 executions** (baseline 10, per-class 10, control 10): no
  `terminated without properly saying goodbye`, no exit 134, no `std::bad_alloc`.
- The failing basemap tests are an independent defect: they fail on their own in a cold process, so they
  are not an isolation problem and this change's mechanism cannot fix them - only mask them again.

Tasks 2.2, 2.3 and everything depending on 2.1 are not started.

### Premise check (extra measurement): an abandoned client breaks every later class

2.1 showed the process boundary cannot land, so the premise the change rests on was measured directly,
by injecting the leak the design found in the sources and seeing whether it really breaks a later class.
Run order of the affected classes in the default configuration:

```
OSMScoutClientDataCacheBudgetTest  ->  OSMScoutClientStyleTest  ->  OSMScoutClientSearchQualityTest  ->  OSMScoutClientFavoriteOrderingTest
```

Treatment - a temporary `@Test` added to `OSMScoutClientDataCacheBudgetTest`, the class immediately
before `OSMScoutClientStyleTest`, that builds a client and deliberately does not close it. Three runs,
identical:

```
Tests run: 233, Failures: 16, Errors: 0, Skipped: 28          (baseline: 232 / 0 / 20)

OSMScoutClientStyleTest        8 of 8 tests fail
  rebuild:63  builder.build() must succeed after closing the previous client ==> expected: not <null>
OSMScoutClientFavoriteOrderingTest   8 of 8 tests fail
  setUp:48    client must be built ==> expected: not <null>
```

Control - the same abandoned client, injected into `OSMScoutClientSearchQualityTest`, which runs
after `OSMScoutClientStyleTest`. Two runs, identical:

```
Tests run: 233, Failures: 8, Errors: 0, Skipped: 27

OSMScoutClientFavoriteOrderingTest   8 of 8 tests fail  (setUp:48, the class after the injection)
OSMScoutClientStyleTest              8 of 8 tests pass  (0.313 s) - unaffected, as its position predicts
```

Conclusion: an abandoned native client breaks exactly the classes that run after it and nothing else,
deterministically (3/3 treatment, 2/2 control). The defect this change is about is therefore real and
demonstrable - it does not fire in a green tree only because firing it needs a test that aborts first,
which is why the 20 shared-config executions of the baseline never showed it.

That leaves two ways to satisfy the isolation requirement, and the measurements choose between them:
releasing the client however the test ends removes the leak (the injected client is exactly the client
those releases would have closed), while a process boundary also removes it but drags in the independent
basemap defect at 7 failing runs in 10.

Both injections were removed; `git status --short JavaScout/src/test/` is empty, so the test sources are
byte-identical to their state before the measurement.

## 3. Client ownership in the test classes

### 3.1 The availability probe is a probe

`TestClients` holds the only two places that construct a client directly:

```
$ grep -rn 'new OSMScoutClient()' JavaScout/src/test/java/
JavaScout/src/test/java/com/framstag/libosmscout/client/TestClients.java:27:  probe = new OSMScoutClient();
JavaScout/src/test/java/com/framstag/libosmscout/client/TestClients.java:46:  return new OSMScoutClient();
```

`assumeNativeLibrary()` builds that probe, releases it in a `finally` and returns nothing to assert
against; `receiverWithoutNativeHandle()` serves the one JNI call that ignores its receiver (the GPX
import). All twelve client-using test classes delegate to the first (the six that had their own
`assumeNativeLibrary()` now call it, the six that built a probe as their client no longer do).

### Defect found during implementation: a class setup method dropped a class from the report

The probe must run per test. With it in `@BeforeAll`, a class that cannot obtain a client was reported
as **0 tests, 0 skipped and no reason at all** - its tests vanished from the report, against the
change's "A class that cannot obtain a client reports its tests as skipped with the reason" and "An
execution reports every test class it started". Same command, before and after moving the probe to
`@BeforeEach`:

```
$ mvn test -Dnative.lib.dir=<missing dir> -Dtest='OSMScoutClientStyleTest,ConfigTest'
before:  Tests run: 0, Failures: 0, Errors: 0, Skipped: 0  -- in ...OSMScoutClientStyleTest
after:   Tests run: 8, Failures: 0, Errors: 0, Skipped: 8  -- in ...OSMScoutClientStyleTest
         Tests run: 9, Failures: 0, Errors: 0, Skipped: 0  -- in com.framstag.libosmscout.ConfigTest
```

The skip reason reaches the report (`target/surefire-reports/TEST-...OSMScoutClientStyleTest.xml`):

```
<skipped type="org.opentest4j.TestAbortedException"><![CDATA[
  org.opentest4j.TestAbortedException: Assumption failed: Native library not available:
  no osmscout_client_java in java.library.path: <missing dir>
    at com.framstag.libosmscout.client.TestClients.assumeNativeLibrary(TestClients.java:29)
```

So a class that cannot obtain a client skips with its reason, and the class after it (ConfigTest) reports
its own outcome.

### 3.2 Release on every path out of a test

| class | release |
|---|---|
| `OSMScoutClientPoiSearchTest` | every test builds its own client, `try`/`finally`; the two multi-client tests released each before building the next |
| `OSMScoutClientGetRoadAtTest` | per test, `try`/`finally`; the two assumption skips (database not openable) happen inside the guarded region |
| `OSMScoutClientDataCacheBudgetTest` | second test kept its `try`/`finally`, the first gained one |
| `OSMScoutClientDataCacheSizeTest`, `OSMScoutClientAdminRegionScopeTest`, `OSMScoutClientSearchQualityTest` | the no-database test gained a `try`/`finally` |
| `SearchReproTest` | all three tests gained a `try`/`finally` (the third had an assumption abort before its `close()`) |
| `OSMScoutClientNavigationTest` | one client for the class, built in `@BeforeAll`, released in `@AfterAll` |
| `OSMScoutClientNavigationLiveTest` | one client per test in an instance field, released in `@AfterEach` |
| `OSMScoutClientImportGpxTest` | receiver per test, released in `@AfterEach` |
| `OSMScoutClientFavoriteOrderingTest`, `FavLocationDialogOrderingTest` | already released in `@AfterEach` |
| `OSMScoutClientStyleTest` | `rebuild()` closes the previous client before building; `@AfterAll` releases the last |
| `OSMScoutClientInstructionDistanceTest` | already released in a `finally` |

Verified by fault injection inside an existing guarded region (`OSMScoutClientPoiSearchTest`, whose four
no-database tests each build a client, so a leak would make the siblings skip with "could not build
client"):

```
injection: assertTrue(false, "temporary fault injection") as the first statement in the try
  -> Tests run: 232, Failures: 1, Skipped: 20    (only the injected failure; siblings unaffected)
injection: Assumptions.assumeTrue(false, "...") at the same position
  -> Tests run: 232, Failures: 0, Skipped: 21    (PoiSearchTest 7 -> 8 skips; siblings still ran)
```

Both injections were removed; the suite is green again at 232 / 0 failures / 20 skipped.

Scope of this evidence: the injections exercise the shape "a client built per test, released in a
`finally`". The classes listed above use that shape or the equivalent `@AfterEach` shape and are covered
by the green suite; they were not each given their own injection.

### 3.3 The compensation loops are gone

`openFixtureClient` and `openLookupClient` (which retried `build()` twenty times with `Thread.sleep(250)`
because a leak could hide a client) are deleted, replaced by `buildClient(lookupDir)` and an
`openFixtureClient(dbDir)` that releases a client it cannot open the database on **before** the test
skips. Ten consecutive suite runs in 6.4 report 0 failures with the loops gone.

## 4. Tests assert the precondition they claim

`OSMScoutClientNavigationTest` builds a real client and asserts `isInitialized()` for every test before
the invalid-handle claim; `OSMScoutClientPoiSearchTest`'s four no-database tests assert
`isInitialized()` before claiming an empty result. Verified by replacing the subject with an
uninitialised client:

```
-> Tests run: 232, Failures: 3, Errors: 0, Skipped: 20
   OSMScoutClientNavigationTest.assumeClient:57  the class needs an initialised client, not the
                                                 uninitialised guard path ==> expected: <true> but was: <false>  (x2)
   OSMScoutClientPoiSearchTest.testZeroRadiusReturnsEmpty:60  the test needs an initialised client
                                                 ==> expected: <true> but was: <false>
```

Exactly the three intended failures, no collateral, skipped unchanged - so the assertions now fail
instead of passing for a reason they do not name. Injection removed.

Unintended but informative: the first version of that injection replaced the class's client without
releasing the real one. That single abandoned client produced **32 failures in one run** -
`OSMScoutClientBasemapConfigTest` 14, `OSMScoutClientFavoriteOrderingTest` 8, `OSMScoutClientStyleTest` 8,
36 skipped - every one of them `builder.build() ... expected: not <null>`. That is the defect
independently reproduced at the class boundary, from a client abandoned by one test class.

`OSMScoutClientImportGpxTest` keeps its test and documents in the class javadoc that its receiver is not
its subject (the native import takes an ignored self argument and parses the file itself), and takes that
receiver from `TestClients.receiverWithoutNativeHandle()`.

## 5. Manual entry point out of the test tree

`JavaScout/src/test/java/com/framstag/libosmscout/DownloadCrashTest.java` moved to
`JavaScout/tools/DownloadCrashTest.java` (`git mv`, so the history follows). Its class javadoc states that
it is not a test, why it lives outside the test source root, the invocation from the repository root and
its prerequisites (native library, client jar, network, a stylesheet directory - the path in `main`
points at this repository).

```
$ rm target/test-classes/com/framstag/libosmscout/DownloadCrashTest.class && mvn -q test-compile
  -> not in the test classpath (no class file produced again)
$ JAR=build-meson/libosmscout-client-java/java/libosmscoutclientjava.jar
  javac -cp "$JAR" -d JavaScout/target/manual-tools JavaScout/tools/DownloadCrashTest.java
  -> compiles cleanly (DownloadCrashTest.class, DownloadCrashTest$1.class)
```

The tool was compiled with the documented command, not run: it downloads maps from a live provider. An
earlier draft of the documented path was wrong (`../build-meson` from `JavaScout/tools`, which resolves
to `JavaScout/build-meson`); the recipe is now written against the repository root and was verified from
there.

## 6. Documentation, regression and integration

### 6.1 Documentation

`JavaScout/AGENTS.md`'s test section now states the canonical command (`JavaScout/test.sh <build-dir>`),
the four rules the suite relies on (release on every path, a probe is never the subject, probes run per
test, the native output corrupts the runner channel) and that `JavaScout/tools/DownloadCrashTest.java` is
a manual tool.

### 6.2 Compilation

```
mvn -q test-compile                  -> clean; the only warning is the pre-existing
                                        "6 problems ... effective model for org.openjfx:javafx-controls:jar:21",
                                        which comes from a dependency description and not from this change
meson compile -C build-meson         -> [415/415] Linking target MCPServer/MCPServer, no error
```

No C++ source was touched by this change.

### 6.3 Existing tests

```
JavaScout/test.sh build-meson             -> 232 run, 0 failures, 0 errors, 20 skipped
ctest -j 2 --output-on-failure (build/)   -> 100% tests passed, 139 of 139, 51.39 s
```

The 20 skipped JavaScout tests are the scenarios that need operator-provided map databases
(`poi.test.db.dir`, `poi.test.multidb.dir`, `nav.test.db.dir`, `search.test.db.dir`) or a JavaFX toolkit,
plus the four no-database tests of `OSMScoutClientPoiSearchTest` that keep their own skip path; the
counts are identical to the pre-change baseline.

### 6.4 Ten consecutive executions and the verdict on the motivating abort

| run | wall | result |
|---|---|---|
| 1 | 11 s | 232 / 0 / 20 |
| 2 | 12 s | 232 / 0 / 20 |
| 3 | 11 s | 232 / 0 / 20 |
| 4 | 11 s | 232 / 0 / 20 |
| 5 | 12 s | 232 / 0 / 20 |
| 6 | 18 s | 232 / 0 / 20 |
| 7 | 20 s | 232 / 0 / 20 |
| 8 | 17 s | 232 / 0 / 20 |
| 9 | 13 s | 232 / 0 / 20 |
| 10 | 14 s | 232 / 0 / 20 |

No abnormal end in any of them: no `terminated without properly saying goodbye`, no exit code 134, no
`std::bad_alloc`, and no `*.dump` file in `target/surefire-reports`. Each run leaves the pre-existing
`*-jvmRun1.dumpstream` of the native output on the runner channel (10 new files, one per run).

Verdict: **the motivating abort did not reproduce - 40 executions in this session** (10 baseline, 10 with
one process per test class, 10 interleaved control, 10 after the change) against a native library built
from revision `71a2cc209`. Its recorded stack predates the fix in `async-worker-lifetime`
(`7f3182a71`: the `MapManager` scan works on a directory snapshot and the destructor stops the worker),
which `MapManager.cpp:36-41` and `:50-53` still carry.

The suite is not claimed to be deterministic: the baseline's 1 differing run in 10 came from the
`OSMScoutClientBasemapConfigTest` wait recorded in 1.2, which this change did not fix and which is now a
`TODO.md` entry.

### 6.5 TODO.md

The entry this change closes ("Two JavaScout classes make a full-suite run abort or fail while each of
them passes on its own") is gone, and a new group names this change and records the two defects it found
and did not fix: the basemap-unload render wait (`OSMScoutClient.cpp:1523-1524`, with the measured rates)
and the native log output corrupting the runner channel (`StreamLogger(std::cout, std::cerr)`,
`LoggerImpl.cpp:115`, with the `forkNode` remedy). The `.gitignore` entry of the group above is
untouched.
