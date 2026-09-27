# Verification — fix-java-jar-consistency

Machine: local development machine, CMake 4.4.3 + Ninja 1.13.2 (`build/`), Meson (`build-meson/`), JDK 17.0.2, Uncrustify 0.83.0.

## 1. CMake jar follows the sources (tasks 1.2–1.6)

| Check | Result |
|---|---|
| 1.2 jar follows a changed Java source | old rule (stashed, pre-change graph): `touch StarredFavoriteLocation.java` → `[1/1] Compiling Java sources and generating JNI headers` ran, jar mtime stayed 16:42:56.178 (defect reproduced). Fixed rule: same touch → `[1/2] Compiling … [2/2] Packaging Java classes into jar`, jar 16:42:45 → 16:42:56 → 16:43:09; `unzip -l` shows `StarredFavoriteLocation.class` |
| 1.3 deleted jar | `rm` jar → `[1/1] Packaging Java classes into jar`, jar produced again |
| 1.4 no-op build stays a no-op | two consecutive `--target java_jar` runs: `ninja: no work to do`, jar mtime unchanged (16:43:13.913) |
| 1.5 install delivers the fresh jar | full-tree `cmake --install build --prefix /tmp/oso-jar-install` aborts in `libosmscout-map-opengl` (pre-existing absolute install destination, see TODO.md); client-java rules verified with `cmake -DCMAKE_INSTALL_PREFIX=/tmp/oso-jar-install -P build/libosmscout-client-java/cmake_install.cmake` → `lib/libosmscoutclientjava.jar`, sha256 equal to the freshly built jar, contains the touched class |
| 1.6 C++ target | object deleted → `[1/1] Building CXX object …` in 21 s, 0 warnings, `libosmscout_client_java.so` relinked |

## 2. Meson source-list parity (tasks 2.1–2.4)

| Check | Result |
|---|---|
| 2.1 both lists | `CurrentRoadInfo.java` added to `native_headers()` (`:4`) and `jar()` (`:41`); `meson compile -C build-meson` rc=0 (11 s, re-run 7 s) |
| 2.2 class present | Meson jar and CMake jar each contain `CurrentRoadInfo.class` |
| 2.3 header generation | generated header sets identical across build systems (4 each: `MapDownloadManager`, `NavigationController`, `OSMScoutClientBuilder`, `OSMScoutClient`); no header for the added class (no native method) |
| 2.4 parity against the sources | 29 `.java` sources ↔ 29 outer classes; both jars contain a class for every source; identical outer-class sets; 4 inner classes each → 33 entries per jar |

Meson compile warnings: pre-existing only (javadoc `no comment`/`no @param` in `BasemapManager.java`, `Conditional on version '>= 0.63.0'` in `libosmscout-client/include/meson.build`) plus one new one, `CurrentRoadInfo.java:47: warning: no @return`, which the file always carried but which only becomes visible once the file is compiled (recorded in TODO.md).

## 3. Extra defect found by the parity check (fixed)

The CMake jar also carried `BasemapManager$BasemapArchive.class`, a class whose source was removed in an earlier commit (the class file dated 19 Sep): `javac` only adds files, so the jar never lost it. The `javac` custom command now removes the classes directory before compiling. After the fix both jars hold 33 identical entries and no class without a source.

## 4. Integration (tasks 3.1–3.5)

- 3.1 `cmake --build build` rc=0 (411 s), 0 compile errors, 66 warnings — all pre-existing and outside the module (Qt 6 deprecations in `InputHandler.cpp`/`MapWidget.cpp`/`Voice*.cpp`, vendored `nanosvg.h`, `MapPainterOpenGL.cpp`, `MapDownloader.cpp`, `GenTextIndex.cpp`; 0 warnings in `libosmscout-client-java`). `meson compile -C build-meson` rc=0.
- 3.2 `ctest -j 2` 136/136 passed (34 s); `meson test --timeout-multiplier 2` 136/136 OK (24 s).
- 3.3 `JavaScout/build.sh <cmake jar>` → BUILD SUCCESS (Maven install of the jar, compile, shade). `JavaScout/test.sh build-meson` → 33 tests, 0 failures, 0 errors, 3 skipped, then the fork aborts (exit 134) attributed to `OSMScoutClientBasemapConfigTest`; that class passes 14/14 when run alone, and identical fork aborts are recorded in `JavaScout/target/surefire-reports/*.dumpstream` from 2026-09-25 and 2026-09-26 15:41 — before this change. Pre-existing, task **3.3 left unchecked**.
- 3.5 `git status --porcelain`: `M TODO.md`, `M libosmscout-client-java/CMakeLists.txt`, `M libosmscout-client-java/java/meson.build`; untracked: the change directory and a pre-existing untracked twin of the JNI source. No other file modified.

## 5. Incident disclosed

While probing the C++ rebuild behaviour, a `sed -i` and a `printf >>` aimed at the JNI source resolved to an **untracked twin** of that file whose name differs from the tracked one by an invisible character (visible as `??` in `git status` before any work in this session; long literals in shell commands are sometimes delivered with a character changed in this environment). The twin was truncated to 1 byte and then restored from the HEAD blob `dd43433f85d500d28f346b4bbffaf11d20394c1d`. Both the tracked file and the twin now hash to exactly the HEAD content (`sha256 cb70c8f8cd16921dc2ed576b3a5a94476aad9a5be2628ca3619d6a36a8f18128`), and `git diff HEAD` for the source is empty: the tracked source was never modified by this change.

## 6. JNI magnification contract (tasks 4.1–4.5, 5.1–5.5)

While verifying 3.3 the JavaScout suite aborted the JVM (`Process Exit Code: 134`). Analysis, in order:

1. The assert was `/usr/include/c++/16/array:219: std::array<osmscout::CellDimension, 26>::operator[] … Assertion '__n < this->size()' failed` (captured with `-DredirectTestOutputToFile=false`).
2. A temporary site-tagged guard in `TileId.cpp` reported the offender: `site=GetTile(mag) level=957 max=26`.
3. Level 957 corresponds to `log2(mag)≈957`, i.e. `mag≈1e288` — and the JNI function read the Java `int magnification` as `jdouble mag`. Commit `182ad5bed` changed the C++ side to a floating-point scale; the Java declarations (and every caller: `JavaScout`'s `int mag`, `FIXTURE_MAG = 15`, `RENDER_ZOOM = 15`) still pass a level. The sibling methods (`getDescription`, `getDescriptionCandidates`, `getObjectBoundingBox`) already take `jint`.
4. A scratch comparison of all Java `native` declarations against the JNI functions found exactly three mismatches: `render`, `renderWithRouteAndPois`, `projectToPixel` (all the same `jint`/`jdouble` position).

Fix: the three entry points take `jint magnificationLevel`, validate `0..CELL_DIMENSION_MAX` and return no result with a warning when outside it; `Magnification` is built from `MagnificationLevel`; and `TileId.cpp` routes every cell-size lookup through one helper that reports an out-of-range level and uses the finest cell size instead of indexing past the table (so an asserts build and a release build agree).

| Check | Result |
|---|---|
| 5.1 full suite, asserts enabled | `./JavaScout/test.sh build-meson` BUILD SUCCESS — 230 tests, 0 failures, 0 errors, 19 skipped (before: 33 tests then exit 134) |
| 5.2 the class that used to abort | `OSMScoutClientBasemapConfigTest` 14/14, before and after the fix |
| 5.3 level validation | the `level=957` report no longer occurs; an out-of-range level yields no result plus `[JNI] render: magnification level … is outside the supported range 0..25` |
| 5.4 C++ regression | `cmake --build build` 0 errors / 0 warnings in the changed files, `ctest` 136/136; `meson compile` + `meson test` 136/136 (asserts enabled) |

Pre-existing and *not* fixed here: a later run of the same suite task aborted with `terminate called after throwing an instance of 'std::bad_alloc'` in `com.framstag.libosmscout.client.SearchReproTest` (client built with `withMapLookupDirectories(".")`, after the map-directory scan). Stashing this change's two source files, rebuilding the Meson library and running that class alone reproduced the same `bad_alloc`, so it is not this change's defect. It is recorded in `TODO.md`.

## 7. Notes

- The two TODO.md entries this change closes now carry "(Closed by `fix-java-jar-consistency`; remove this entry when that change is archived.)".
- Findings recorded in TODO.md instead of being fixed here: the OpenGL absolute install destination, the never-cleaned JNI header directory, the `CurrentRoadInfo` javadoc warning, the untracked twin of the JNI source, the six JNI functions without a Java declaration, and the pre-existing `std::bad_alloc` abort of `SearchReproTest`.

## 8. Magnification as a fractional scale (revision of §6)

The first attempt aligned the bridge to the Java `int` level (the parameter the Java declarations named). NaviVeylin — whose own binding uses `double magnification` in `Android/OsmScoutLib` (`osm.scout.Database`, `Projection`, `MercatorProjection`) — introduced fractional zoom levels, so the bridge keeps the scale factor and the Java API follows it:

- `render`, `renderWithRouteAndPois`, the `renderWithRoute` overload and `projectToPixel` take and document `double magnification` (scale factor, 2^zoom level).
- Before a `Magnification` is constructed, the entry points require a finite scale `>= 1` whose `floor(log2(scale))` is within `CELL_DIMENSION_MAX` (25); otherwise they warn and return no result. This is what turned the previous abort (`level=957`) into a refused call.
- `JavaScout`'s `MapRenderer` (both call sites) and `OSMScoutClientDataCacheSizeTest` pass `Math.pow(2, level)`; `OSMScoutClientBasemapConfigTest`'s retry uses `Math.pow(2, FIXTURE_MAG + 0.25)`, so a fractional scale goes through the render path.

Verification of this revision:

| Check | Result |
|---|---|
| JNI suite (Meson library, asserts on), `SearchReproTest` excluded | BUILD SUCCESS — 227 tests, 0 failures, 0 errors, 18 skipped |
| JNI suite with `-Dpoi.test.db.dir`/`-Dsearch.test.db.dir` pointing at `maps/Dortmund` | 227 tests, 3 failures — all “database should be loaded” timeouts in the DB-property tests; `maps/Dortmund` has no `db.json`, so the provided map is not registerable. Unrelated to the magnification change |
| C++ suites | `ctest` 139 tests: 137 passed + the new signature test in each build system, 0 failures; `meson test` the same |
| Signature check | `scripts/check-jni-signatures.sh`: 55 native declarations match; on a copy with one parameter flipped to `int`, it reports `MISMATCH: projectToPixel` and exits 1 |
