# Verification

Unit: `client-java-search-request-lifecycle` (branch `client-java-search-request-lifecycle`, base
`master`). Every command below was run on this machine in `/home/tim/projects/libosmscout`; the
output quoted is the output the command produced.

## 1. The registry test (CMake)

```
cmake --build /home/tim/projects/libosmscout/build --target SearchBreakerTest -- -j8
```
Reconfigured the build directory (the new target comes from `Tests/CMakeLists.txt`), then
`Building CXX object Tests/CMakeFiles/SearchBreakerTest.dir/src/SearchBreakerTest.cpp.o` and
`Linking CXX executable Tests/SearchBreakerTest-1.1.1`. Build clean, no warning from the new
translation unit.

```
/home/tim/projects/libosmscout/build/Tests/SearchBreakerTest
```
`All tests passed (34 assertions in 9 test cases)`

```
ctest --test-dir /home/tim/projects/libosmscout/build -R SearchBreakerTest --output-on-failure
```
`1/1 Test #67: SearchBreakerTest ... Passed     100% tests passed out of 1`

## 2. The registry test (Meson)

The existing Meson build directory is `build-meson-check`.

```
ninja -C /home/tim/projects/libosmscout/build-meson-check Tests/SearchBreakerTest
```
`[1/2] Compiling C++ object Tests/SearchBreakerTest.p/src_SearchBreakerTest.cpp.o`,
`[2/2] Linking target Tests/SearchBreakerTest`. (`meson compile -C … SearchBreakerTest` reports
"target not found" until the directory has regenerated; the first `ninja` invocation regenerated
it, `meson test` below then found the case by name.)

```
/home/tim/projects/libosmscout/build-meson-check/Tests/SearchBreakerTest
```
`All tests passed (34 assertions in 9 test cases)`

```
meson test -C /home/tim/projects/libosmscout/build-meson-check --print-errorlogs "Check search breaker"
```
`1/1 libosmscout:Check search breaker OK  0.01s` — `Ok: 1  Fail: 0`

## 3. The native library and the jar

```
cmake --build /home/tim/projects/libosmscout/build --target osmscout_client_java -- -j8
```
`[2/3] Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`,
`[3/3] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`. The touched
translation unit compiles without a new warning.

```
cmake --build /home/tim/projects/libosmscout/build --target java_jar -- -j8
```
`[1/1] Packaging Java classes into jar`.

```
javap -classpath /home/tim/projects/libosmscout/build/libosmscout-client-java/libosmscoutclientjava.jar \
      com.framstag.libosmscout.client.OSMScoutClient | grep -E "beginSearch|cancelSearch|searchLocations|searchPOIsByTypes|SEARCH_SLOT"
```
shows `SEARCH_SLOT_PLACES/POI/ROUTE/CAR`, `searchLocations(String,int,long,long)`,
`searchLocations(String,int,String,long)`, the two convenience overloads, `beginSearch(String)`,
`cancelSearch(long)` and `searchPOIsByTypes(…,long)`; the parameterless `cancelSearch()` and the
boolean-cancel overload are gone.

```
scripts/check-jni-signatures.sh
```
`JNI signatures match: 60 native declarations checked, 5 dead JNI functions reported above.`
The five warnings (`getMaxSpeedAt`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox`,
`searchLocationByForm(jstring,jstring,jstring,jstring,jint,jlong)`) are the pre-existing dead
bridge code that the script reports as a warning rather than a failure; `searchLocationByForm` is
TODO.md item 310 and keeps its C++ parameter list aligned with the reference.

## 4. The ported Java API compiles, and did not before the port

The fresh jar (47593 bytes) was installed into the local Maven repository and its size compared with
the built jar:

```
./JavaScout/mvnw -f JavaScout/pom.xml install:install-file \
  -Dfile=/home/tim/projects/libosmscout/build/libosmscout-client-java/libosmscoutclientjava.jar \
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT \
  -Dpackaging=jar -q
stat -c '%s %n' ~/.m2/…/libosmscout-client-java-1.0-SNAPSHOT.jar build/libosmscout-client-java/libosmscoutclientjava.jar
```
`47593` both — the installed jar is the freshly built one.

Not ported yet (the four JavaScout files temporarily restored to `master` with
`git stash push -- <the four paths>`, then popped again), against the new jar:

```
./JavaScout/mvnw -f JavaScout/pom.xml -DskipTests package
```
```
[ERROR] SearchOverlay.java:[181,19] method cancelSearch in class com.framstag.libosmscout.client.OSMScoutClient cannot be applied to given types; required: long
[ERROR] SearchOverlay.java:[563,49] no suitable method found for searchLocations(java.lang.String,int,java.lang.String,boolean)
[ERROR] FavLocationDialog.java:[668,81] incompatible types: long cannot be converted to java.lang.String
[INFO] BUILD FAILURE
```

Ported (all four files as in this branch):

```
./JavaScout/mvnw -f JavaScout/pom.xml -DskipTests package
```
`BUILD SUCCESS` — main sources compiled (`Compiling 25 source files with javac`).

```
./JavaScout/mvnw -f JavaScout/pom.xml clean test-compile
```
`Compiling 36 source files with javac [debug target 17] to target/test-classes`, `BUILD SUCCESS`.
So every in-repo caller of the changed Java API is ported: main and test sources compile.

## 5. JavaScout tests against the real database

The native library is at `build/libosmscout-client-java` (there is no `build/lib` in this checkout);
`maps/nordrhein-westfalen/db.json` carries `typeConfigVersion: 27`, so the v27 database is present.

```
xvfb-run -a ./JavaScout/mvnw -f JavaScout/pom.xml test -Dtest=OSMScoutClientAdminRegionScopeTest \
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java \
  -Dsearch.test.db.dir=/home/tim/projects/libosmscout/maps/nordrhein-westfalen
```
`Tests run: 2, Failures: 0, Errors: 0, Skipped: 0`. This is the end-to-end probe of the new API:
`beginSearch(SEARCH_SLOT_PLACES)` → `searchLocations(regionName, 20, handle, generation)` →
`cancelSearch(generation)` in a `finally`, against the Nordrhein-Westfalen database.

```
xvfb-run -a ./JavaScout/mvnw -f JavaScout/pom.xml test \
  -Dtest='OSMScoutClientSearchQualityTest,SearchReproTest,OSMScoutClientDataCacheSizeTest' \
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java \
  -Dsearch.test.db.dir=/home/tim/projects/libosmscout/maps/nordrhein-westfalen
```
`Tests run: 8, Failures: 0, Errors: 0, Skipped: 0` (SearchReproTest 3, SearchQualityTest 3,
DataCacheSizeTest 2).

The whole JavaScout suite, same properties:

```
xvfb-run -a ./JavaScout/mvnw -f JavaScout/pom.xml test \
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java \
  -Dsearch.test.db.dir=/home/tim/projects/libosmscout/maps/nordrhein-westfalen
```
First run: 2 failures —
`OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads` (the documented
intermittent flake) and `OSMScoutClientStyleTest.testLoadStyleSheetUnloadableKeepsPreviousStyle`
(a poll-with-timeout against `Tests/data/testregion`). Second run of the same suite:
`Tests run: 267, Failures: 0, Errors: 0, Skipped: 18` — `BUILD SUCCESS`. Run in isolation both pass:
`OSMScoutClientBasemapConfigTest` `14/14`, `OSMScoutClientStyleTest` `8/8`. Neither test touches the
search machinery, and the diff touches no style or basemap file.

A `master` baseline count was not taken (see "Not verified"); the branch's second full run is
green, so there is nothing to attribute to this change.

## 6. The change directory

```
openspec validate client-java-search-request-lifecycle --strict
```
`Change 'client-java-search-request-lifecycle' is valid`

## 7. Diff hygiene

```
git status --short
```
Modified: the four JavaScout files, `Tests/CMakeLists.txt`, `Tests/meson.build`,
`libosmscout-client-java/java/…/OSMScoutClient.java`, `libosmscout-client-java/src/OSMScoutClient.cpp`.
New: `Tests/src/SearchBreakerTest.cpp`, `libosmscout-client-java/src/search_breaker.h`,
`openspec/changes/client-java-search-request-lifecycle/`. Nothing else.

```
grep -n "route_failure_kind\|KIND_\|onErrorDetail\|RoutingError\|SetLabelLine" libosmscout-client-java/src/OSMScoutClient.cpp
```
No match — the routing-failure hunks of `dff960e34` and the label-bounds hunk of `e303556e2`
are not in this branch. `setDescriptionLanguage` and the method-lookup resilience are untouched
(only the search hunks appear in `git diff origin/master -- libosmscout-client-java/src/OSMScoutClient.cpp`).

## Not verified

- **No `master` baseline of the JavaScout suite.** The ported branch was run twice (2 flakes, then
  267/0/0); a `master` run would need the old JavaScout sources compiled against an old jar, i.e. a
  separate checkout. The branch run is green, so no regression is claimed either way.
- **No device/projection session.** The per-slot independence (car vs phone) is covered by the host
  test `SearchBreakerTest` and by the registry's design, not by an on-device run.
- **`scripts/format-check.sh`** was not used as a gate: it is red repo-wide on this machine,
  including pristine `master` files. Style was matched by hand against the surrounding code.
- **`searchLocationByForm`** still has no Java declaration (TODO.md item 310), so its generation
  parameter is unreachable from Java in this change; only the C++ list changed, as in the reference.
