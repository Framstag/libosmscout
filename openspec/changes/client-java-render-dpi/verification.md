# Verification — client-java-render-dpi

Machine: local development machine, CMake + Ninja (`build/`, Release, `OSMSCOUT_BUILD_CLIENT_JAVA=ON`),
Meson (`build-meson/`), JDK 17.0.2, Maven 3.x (offline), JUnit 5.

Baseline: `git rev-parse master origin/master` → `0205b359e62a669cd4a84686478e2e49dbd9f927` for both; the
branch `client-java-render-dpi` was created from `origin/master`.

## 1. The bridge and the declarations (tasks 1.1–2.3)

`git diff --stat` over the change:

```
libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java | 73 ++++++++++++++++--
libosmscout-client-java/src/OSMScoutClient.cpp                                   | 45 ++++++-------
2 files changed, 88 insertions(+), 30 deletions(-)
```

| Check | Result |
|---|---|
| 1.1 both JNI entries take the DPI | `render(…, jdouble magnificationScale, jdouble dpi)` forwards `dpi` into `renderWithRouteAndPois(…, jdouble dpi, …)`; the JNI names are unchanged |
| 1.2 fallback, no rejection | the render body no longer assigns `dpi` from `Settings::GetMapDPI()` unconditionally; the only read of the configured value is `if (!(dpi > 0.0)) { dpi = data->settings ? data->settings->GetMapDPI() : 96.0; }` — `!(x > 0.0)` is true for `NaN` and for `<= 0`, and there is no early return for it |
| 1.3 setter removed | the `Java_…_setMapDpi` function and its comment block are gone; `git grep -n setMapDpi libosmscout-client-java` finds nothing |
| 2.1 declarations carry the DPI | `public native int[] render(int, int, double, double, double, double, double)` and `public native int[] renderWithRouteAndPois(int, int, double, double, double, double, double, double[], double[], double[], double[], double, double, double[], double[])`, both with `@param dpi … Double.NaN or a non-positive value renders with the DPI configured on the client` |
| 2.2 non-native forwarders | the 6-argument `render` and the 14-argument `renderWithRouteAndPois` are plain `public` methods that forward with `Double.NaN`; `renderWithRoute()` keeps calling the 14-argument form |
| 2.3 Javadoc cross references | the three links that named a list which does not exist (`setBasemapLookupDirectory`, `renderWithRouteAndPois`, `renderWithRoute`) now name declared lists |

## 2. Signature parity and both build systems (tasks 2.4, 3.1, 3.2, 3.6)

| Command | Result |
|---|---|
| `bash scripts/check-jni-signatures.sh` | exit 0 — `JNI signatures match: 55 native declarations checked, 5 dead JNI functions reported above.` The dead list is now `getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox` (was six, `setMapDpi` is gone) |
| `cmake --build build --target osmscout_client_java java_jar` | rc=0 — `Compiling Java sources and generating JNI headers`, `Packaging Java classes into jar`, `Building CXX object …/OSMScoutClient.cpp.o`, `Linking CXX shared library …/libosmscout_client_java.so`; no warning from the touched C++ file |
| `cd build && ctest -R JniSignatureParityTest --output-on-failure` | `1/1 Test #46: JniSignatureParityTest … Passed 0.21 sec` |
| `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` | rc=0 — the JAR and `libosmscout_client_java.so.1.1.1` are relinked; the javadoc warnings are pre-existing (69, none in `OSMScoutClient.java`) |
| `meson test -C build-meson --list \| grep -i jni` | `libosmscout:Check JNI signature parity` |
| `meson test -C build-meson "Check JNI signature parity"` | `1/1 libosmscout:Check JNI signature parity OK 0.44s`, `Ok: 1 Fail: 0` |
| `openspec validate client-java-render-dpi --strict` | exits 0 (change is valid) |

Java sources compiled against the changed declarations: the CMake `java_jar` step compiles every Java
source of the client library (the forwarders included), and `JavaScout` was compiled as a consumer with
`cd JavaScout && mvn -o -q -DskipTests compile` against the freshly built JAR — rc=0, so no call site of
`JavaScout`'s `MapRenderer` or of the tests had to change.

## 3. The frame contract at Java level (task 3.3)

The change adds no Java-level test (its Impact section names no test file), so the contract was observed
with a throw-away Java program — not committed, removed after the run — that builds one real JNI client and
compares frames:

```java
OSMScoutClient c = new OSMScoutClientBuilder()
    .withMapLookupDirectories("…/Tests/data/testregion")
    .withStyleSheetDirectory("…/stylesheets")
    .withPhysicalDpi(96.0).withUnits("metrics").build();
// 64x48 at 50.4114/14.5286, magnification 2^15
int[] noDpi = c.render(w, h, lat, lon, 0.0, mag);                     // 6-argument forwarder
int[] p96   = c.render(w, h, lat, lon, 0.0, mag, 96.0);
int[] p400  = c.render(w, h, lat, lon, 0.0, mag, 400.0);
int[] pNaN  = c.render(w, h, lat, lon, 0.0, mag, Double.NaN);
int[] pNeg  = c.render(w, h, lat, lon, 0.0, mag, -1.0);
int[] again = c.render(w, h, lat, lon, 0.0, mag);                     // after the 400-dpi request
```

Run with `java -cp <probe>:build/libosmscout-client-java/libosmscoutclientjava.jar
-Djava.library.path=build/libosmscout-client-java DpiProbe`:

```
RENDER_NO_DPI=3072          P96=3072  P400=3072  PNaN=3072  PNEG=3072  P_NO_DPI_AGAIN=3072
NO_DPI_EQUALS_96=true       NAN_EQUALS_96=true    NEG_EQUALS_96=true
NO_DPI_STILL_96_AFTER_400=true
PIXELS_DIFFERING_96_VS_400=1400/3072
PIXELS_DIFFERING_NO_DPI_VS_400=1400/3072
```

Read: a stated DPI reaches the projection (`96` vs `400` differ in 1400 of 3072 pixels), an undefined or
non-positive DPI is not rejected and renders exactly the configured-DPI frame, a request without a DPI
renders exactly the previous behaviour, and a 400-dpi request does not change what a later no-DPI request
projects with. Every returned frame has the requested size (`64 * 48 = 3072`).

Map availability, as asked: `maps/` does contain a routable database (`maps/arnsberg-regbez`, with
`intersections.dat`, `ptroutes.dat`, `areas.dat`, `nodes.dat`), but the tree refuses it —
`File '…/types.dat' does not have the expected format version! Actual 26, expected: 27` — so the probe used
the repository fixture `Tests/data/testregion`, which is also what `OSMScoutClientBasemapConfigTest` renders.

## 4. Java suite — no regression (task 3.4)

`cd JavaScout && JAVASCOUT_MAP_DIR=…/maps/arnsberg-regbez mvn -o test
-Dnative.lib.dir=…/build-meson/libosmscout-client-java/src`

| Run | Result |
|---|---|
| with this change | `Tests run: 230, Failures: 19, Errors: 0, Skipped: 25` — `OSMScoutClientStyleTest` 8, `OSMScoutClientFavoriteOrderingTest` 8, `OSMScoutClientInstructionDistanceTest` 1, `OSMScoutClientAdminRegionScopeTest` 1, `OSMScoutClientNavigationLiveTest` 1 |
| baseline: this change stashed (`git stash push`), `meson compile … osmscout_client_java libosmscoutclientjava`, JAR reinstalled, same command | `Tests run: 230, Failures: 19, Errors: 0, Skipped: 25` — the identical 19 failures |

The failure messages are environment-bound and unrelated to the render entry points: the style and
favorites classes fail on `builder.build() must succeed after closing the previous client` (and pass in
isolation in both states: `-Dtest=OSMScoutClientStyleTest,OSMScoutClientFavoriteOrderingTest` → 16 tests,
0 failures, on the baseline), and the database-driven classes wait for `maps/arnsberg-regbez`, which cannot
be opened at format version 26. Identical failure sets before and after ⇒ no regression from this change.

Not run (tasks 3.3 as a repository test, and 3.5): see the notes on those tasks in `tasks.md`.

## 5. The BREAKING claim, corrected

`git grep -n setMapDpi origin/master -- libosmscout-client-java` finds the JNI function and its comment
block only (`src/OSMScoutClient.cpp:1080,1090,1096`); there is no declaration in `OSMScoutClient.java` and
no Java source calls it anywhere in the tree. `master`'s `TODO.md` (JNI backlog entry) likewise lists
`setMapDpi(jdouble)` among six JNI functions "without a Java declaration — dead bridge code that no Java
caller can reach". The check's own output confirms it: `WARNING: JNI function without a Java declaration:
setMapDpi (jdouble)`, 6 dead before and 5 after.

So removing it breaks no Java source, and `proposal.md` (dropped the `**BREAKING**` marker, stated the
fact) and `design.md` (D3, Risks, Migration Plan) were corrected to match. The configured DPI — the
builder's `withPhysicalDpi` — remains as the fallback, unchanged.

## 6. Notes

- `TODO.md`'s JNI backlog entry still says six dead functions including `setMapDpi`; it now overstates the
  count by one (five remain). It was left untouched because the proposal's Impact section lists no
  `TODO.md`, and task 4.3 keeps the diff to that list.
- No map, database or style file changed; no file format version bump applies (`guidelines/FileFormatVersion.md`).
