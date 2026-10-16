# Verification

Change: `client-java-type-icon-entry-point` — the Java client gains an entry point that returns the artwork
the active stylesheet gives one object type, into a square, without a map frame.

## 1. What was implemented

`libosmscout-client-java/src/OSMScoutClient.cpp`, one self-contained block inserted after `getAddressAt` and
before `getMaxSpeedAt`:

- the includes the entry point needs: `<osmscoutmapcairo/LoaderPNG.h>` and
  `<osmscoutmapcairo/SymbolRendererCairo.h>` inside the `HAVE_MAP_CAIRO` block that holds `<cairo.h>`, and
  `<osmscoutmap/Styles.h>` next to the other `osmscoutmap` includes;
- the constants `NAVIVEYLIN_TYPE_ICON_MAGNIFICATION_LEVEL` (`osmscout::Magnification::magVeryClose`),
  `NAVIVEYLIN_TYPE_ICON_DPI` (96.0) and `NAVIVEYLIN_TYPE_ICON_MAX_SIZE` (512);
- `DrawTypeIconSymbol` and `DrawTypeIconImage` under `#if defined(HAVE_MAP_CAIRO)`, both scaling through
  `naviveylin::TypeIconFitScale` and both returning false when there is nothing to draw;
- `Java_com_framstag_libosmscout_client_OSMScoutClient_renderTypeIcon`, ported from the reference
  implementation (`origin/naviveylin-local`, commits `b199bdf8f` and `f3d7e046e`): the no-Cairo warning, the
  client-data and request guards, the synchronous job over the loaded databases with the basemap appended, the
  type lookup, the tag-less `FeatureValueBuffer`, the own projection at the icon magnification, the
  node-then-area icon style lookup, the surface, the symbol-versus-image order driven by
  `preferSymbolIcons`, the per-stage warnings and the `int[]` return. The per-pixel conversion calls
  `naviveylin::TypeIconArgbWord` from the new helper instead of repeating the arithmetic inline.

`libosmscout-client-java/src/type_icon_pixels.h` is new: the dependency-free helper
(`naviveylin::TypeIconFitFraction`, `TypeIconFitScale`, `TypeIconArgbWord`), following the
`frame_pixel_layout.h` / `route_length.h` pattern. The entry point keeps no copy of the fit fraction.

`libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` gains one Javadoc block and
`public native int[] renderTypeIcon(String objectType, int sizePx);` after `projectToPixel`.

`Tests/src/TypeIconPixelsTest.cpp` is new and registered in `Tests/CMakeLists.txt` and `Tests/meson.build`.

### Deviation from the reference implementation

The spec delta requires every stage warning to name the stage, the type and the requested size. The reference
implementation logs `px=` only in the stages after the projection exists, so the three warnings that decide
before it (`no-style-config`, `no-type-config`, `unknown-type`) carry the size here as well. Nothing else
about them changes; the size is a parameter of the request and is known at every stage. No spec requirement
was reworded.

## 2. CMake build and test (exact commands and results)

- `cmake --build build --target osmscout_client_java java_jar TypeIconPixelsTest` — exit 0.
  `[6/8] Building CXX object libosmscout-client-java/CMakeFiles/osmscout_client_java.dir/src/OSMScoutClient.cpp.o`,
  `[7/8] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so`,
  `[2/8] Building CXX object Tests/CMakeFiles/TypeIconPixelsTest.dir/src/TypeIconPixelsTest.cpp.o`,
  `[4/8] Linking CXX executable Tests/TypeIconPixelsTest-1.1.1`, `[1/8] Compiling Java sources and generating
  JNI headers`, `[3/8] Packaging Java classes into jar`. No compiler warning from either touched file; the only
  warnings in the log are the configure-time `FindPango`/`FindGLIB2`/`FindGObject` author warnings, which are
  unrelated and pre-existing.
- `ctest --test-dir build -R TypeIconPixelsTest --output-on-failure` — `1/1 Test #68: TypeIconPixelsTest
  ... Passed 0.01 sec`, `100% tests passed out of 1`.
- `./build/Tests/TypeIconPixelsTest` — `All tests passed (24 assertions in 5 test cases)`.
- `ctest --test-dir build -j 8 -E Performance --output-on-failure` — `100% tests passed out of 117`, 117
  started, 0 failed, `Total Test time (real) = 21.68 sec`. This includes the parity test that runs the JNI
  signature script (`#43 JniSignatureParityTest ... Passed`). The build and this run were repeated after the
  last edit to `OSMScoutClient.cpp` (the requested size in the three stage warnings) and report the same
  counts.
- `bash scripts/check-jni-signatures.sh` — exit 0,
  `JNI signatures match: 58 native declarations checked, 5 dead JNI functions reported above.`
  Before this change the same command reported `57 native declarations checked, 5 dead JNI functions` — the
  count goes up by one (the new declaration) and the entry point is *not* among the five reported dead
  functions, which are unchanged and pre-existing (`getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`,
  `getAddressAt`, `getDatabaseBoundingBox`).
- `javap -cp build/libosmscout-client-java/libosmscoutclientjava.jar
  com.framstag.libosmscout.client.OSMScoutClient` — the built jar lists
  `public native int[] renderTypeIcon(java.lang.String, int);` directly after `public native double[]
  projectToPixel(...)`, i.e. the declaration is compiled into the artifact the callers consume.

## 3. Meson build and test (exact commands and results)

- `meson setup --reconfigure debug` — reconfigured, so the new test target is picked up
  (`meson introspect debug --targets` then lists `TypeIconPixelsTest | executable` next to
  `libosmscoutclientjava | jar` and `osmscout_client_java | shared library`).
- `meson compile -C debug osmscout_client_java libosmscoutclientjava TypeIconPixelsTest` — `[7/10] Compiling
  C++ object Tests/TypeIconPixelsTest.p/src_TypeIconPixelsTest.cpp.o`, `[8/10] Linking target
  Tests/TypeIconPixelsTest`, `[9/10] Compiling C++ object
  libosmscout-client-java/src/libosmscout_client_java.so.1.1.1.p/OSMScoutClient.cpp.o`, `[10/10] Linking
  target libosmscout-client-java/src/libosmscout_client_java.so.1.1.1`, `[6/10] Creating JAR
  libosmscout-client-java/java/libosmscoutclientjava.jar`; ninja exited 0 with no compilation warning from a
  touched file. The Java step's pre-existing javadoc warnings (`BasemapManager.java`, `PoiCategories.java`)
  are unchanged by this change and come from files that are not part of it.
- `meson test -C debug --print-errorlogs "Check type icon pixels"` — `1/1 libosmscout:Check type icon pixels
  OK 0.01s`, `Ok: 1`, `Fail: 0`.
- Not run: the whole meson test suite. The CMake suite covers the same tests on this machine, and a bare
  `meson compile -C debug` (which the suite's fixtures assume) is not a safe command in this shared build
  directory; only the three targets this change touches were compiled and only the new test was run.

## 4. OpenSpec

- `openspec validate client-java-type-icon-entry-point --strict` — `Change 'client-java-type-icon-entry-point'
  is valid`. The delta's requirements are the orchestrator's and were not reworded; requirement descriptions
  stay below strict mode's 500-character limit and the detail lives in `design.md`.

## 5. JavaScout suite

- `mvn -o -q install:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT
  -Dpackaging=jar` — no output, exit 0 (the freshly built jar replaces the local artifact).
- `mvn -o -f JavaScout/pom.xml test-compile
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java` — `BUILD SUCCESS`.
- `mvn -o -f JavaScout/pom.xml test
  -Dnative.lib.dir=/home/tim/projects/libosmscout/build/libosmscout-client-java` — `BUILD SUCCESS`,
  `Tests run: 230, Failures: 0, Errors: 0, Skipped: 19`. The suite ran fully green here, including
  `RouteInstructionTest` (which a sibling branch's stale installed artifact used to break). No test of the
  suite exercises `renderTypeIcon`: none of them can open a v27 database, and the entry point needs one (see
  section 6).

## 6. Testing option chosen, and what could not be run

- **Chosen: the dependency-free helper plus a host Catch2 test, and the entry point verified by building it
  and by inspection.** The two decisions the entry point makes in arithmetic are the fit scale and the
  conversion of a surface pixel into the word Java receives. Factoring them into
  `libosmscout-client-java/src/type_icon_pixels.h` makes them testable on a host with no database, no style
  and no Cairo surface, and `Tests/src/TypeIconPixelsTest.cpp` pins them with values that a wrong
  implementation could not produce: three distinct fit factors that each put the longer side of the artwork at
  0.9 of the square, zero for a missing extent or side, and the exact `0x80FD0000` word of a premultiplied
  half-alpha red (which is neither the premultiplied hand-over `0x807F0000` nor a saturated `0x80FF0000`).
- **Not run: the entry point itself, end to end.** `maps/` holds format v26 databases while the library
  expects v27, so no database whose type configuration and stylesheet match this library can be opened in this
  tree, and a v27 import is out of scope and was not attempted. A Java call of `renderTypeIcon` would therefore
  return `null` before reaching a stylesheet, and a test over it would assert nothing about the artwork. What
  is verified instead: the entry point compiles into the bridge (section 2), the Java declaration is in the
  jar and in the parity check (sections 2), its two pure decisions are covered by the host test (section 2),
  and its request and stage contract is inspected against the spec delta's scenarios (the guards, the
  candidate list with the basemap fallback, the tag-less feature buffer, the node-then-area lookup, the silent
  "no icon for this type" case and the five stage warnings that each carry the stage, the type and the size).
- **Not run: a rendering assertion over a real stylesheet icon.** It would need the same v27 database, and the
  symbol-versus-image order it would cover is already pinned for the map painter by
  `Tests/src/MapPainterIconSymbolPreferenceTest.cpp`; the entry point implements the same rule.

## 7. Diff hygiene

- The change touches exactly `libosmscout-client-java/src/OSMScoutClient.cpp` (insertions only),
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`,
  `libosmscout-client-java/src/type_icon_pixels.h`, `Tests/src/TypeIconPixelsTest.cpp`,
  `Tests/CMakeLists.txt`, `Tests/meson.build` and `openspec/changes/client-java-type-icon-entry-point/`.
- `git status --short` after the commit lists only the pre-existing untracked entries (`maps/repository/` and
  the sibling `openspec/changes/client-java-*` directories), which are never staged: no `git add -A`, no
  `git add .`, no `git commit -a`.
- `scripts/format-check.sh` was not used as a gate: it is red repo-wide on this machine, including on pristine
  `master` files, so the touched lines follow the surrounding style by hand instead.

## Host verification 2026-10-16 (map-dependent items)

Task 4.6 is now ticked: a Java call renders real types' icons from the shipped stylesheets, on the v27
database `maps/nordrhein-westfalen` with `stylesheets/` configured.

```bash
# compiled once into build/loop-probes-2026-10-16
java -Djava.library.path=build/libosmscout-client-java \
  -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/loop-probes-2026-10-16 \
  TypeIconProbe maps/nordrhein-westfalen stylesheets libosmscout/data/icons/14x14/standard
```

| stylesheet | type | returned pixels |
|---|---|---|
| `standard` | `amenity_parking` (an `AREA.ICON` type, `stylesheets/include/amenity.oss`) | `1024` (`32x32`), `516` painted, bbox `[3,1 .. 28,30]`, first painted `(3,1)=0x0E0091DA`, centre `0xC90894D8` |
| `standard` | `amenity_fire_station_building` | `1024`, `1024` painted, `900` opaque, centre `0xFFFFFFFF` |
| `railways` | `railway_station` (`AREA.ICON` in `stylesheets/railways.oss`) | `1024`, `708` painted, bbox `[1,1 .. 30,30]`, centre `0x5500FF00` |
| `standard` | `highway_roundabout` (no icon in any shipped stylesheet) | `null` |

Read: the entry point resolves a real type's icon style from a real stylesheet and returns the painted
`0xAARRGGBB` pixels (semi-transparent anti-aliased edges included); a type the stylesheets give no icon
returns `null`. **What stayed unverified:** the symbol-versus-image precedence when a stylesheet names both
for one type (`amenity_parking` names symbol `amenity_parking` and image `parking`; only the default order was
observed), and the Android-side consumption of these pixels.

Log: `evidence/type-icon-probe-nrw-2026-10-16.log` (command, UTC timestamp, exit 0, revision `06b197a35`).
Probe source: `evidence/TypeIconProbe.java`.
