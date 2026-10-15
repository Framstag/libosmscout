# Tasks

## 1. The JNI entry point (spec: map-rendering)

- [x] 1.1 Add the includes the entry point needs to `libosmscout-client-java/src/OSMScoutClient.cpp`:
  `<osmscoutmapcairo/LoaderPNG.h>` and `<osmscoutmapcairo/SymbolRendererCairo.h>` inside the existing
  `#ifdef HAVE_MAP_CAIRO` block next to `<cairo.h>`, and `<osmscoutmap/Styles.h>` next to the other
  `osmscoutmap` includes. Verify: the file compiles with `HAVE_MAP_CAIRO` (the CMake target builds), and both
  Cairo headers exist under `libosmscout-map-cairo/include/osmscoutmapcairo/`.
- [x] 1.2 Add the constants the entry point uses, with the reference implementation's explanations:
  `NAVIVEYLIN_TYPE_ICON_MAGNIFICATION_LEVEL` (`osmscout::Magnification::magVeryClose`, the *level*
  constructor), `NAVIVEYLIN_TYPE_ICON_DPI` (96.0, the projection's reference resolution) and
  `NAVIVEYLIN_TYPE_ICON_MAX_SIZE` (512, the largest accepted square). Verify: the magnification level is the
  one the entry point constructs its projection with, not `Magnification(16.0)` (the factor constructor, level
  4, at which no stylesheet defines a POI icon).
- [x] 1.3 Add the two drawing helpers under `#if defined(HAVE_MAP_CAIRO)`, `DrawTypeIconSymbol(cairo_t*,
  const osmscout::Projection&, const osmscout::Symbol&, int side)` and `DrawTypeIconImage(cairo_t*,
  const std::string &iconDirectory, const std::string &iconName, int side)`, with the reference
  implementation's comments (the loader concatenates path and name, so the directory needs its separator; a
  stylesheet may name artwork that ships as SVG only, which is a normal outcome). Verify: both return false
  when there is nothing to draw (no extent, an empty directory or name, an unloadable or zero-sized image)
  instead of drawing something, and the caller falls back to the other rendering for that.
- [x] 1.4 Add `Java_com_framstag_libosmscout_client_OSMScoutClient_renderTypeIcon` after `getAddressAt` and
  before `getMaxSpeedAt`, ported as it stands: the `#if !defined(HAVE_MAP_CAIRO)` early return with a warning;
  the `getClientData`/`dbThread` guard; the `objectType == nullptr || sizePx <= 0 || sizePx >
  NAVIVEYLIN_TYPE_ICON_MAX_SIZE` guard; `RunSynchronousJob` over the client's databases with the basemap
  appended as a fallback candidate; the type lookup in the database's type config; a `FeatureValueBuffer` that
  carries the type and no tag values; the icon magnification and the entry point's own `MercatorProjection`;
  `GetNodeIconStyle` then `GetAreaIconStyle`; the visibility check; the cairo surface and context; the
  symbol-versus-image order driven by `data->preferSymbolIcons` (the preference *prefers* the symbol, it does
  not disable it for a symbol-only entry); the per-stage warnings; and the `NewIntArray` /
  `SetIntArrayRegion` return. Verify: no other function in the file changes (the diff is insertions only),
  and `master`'s newer content in the same file is untouched.
- [x] 1.5 Call the helper header's conversion instead of the reference implementation's inline `argb` loop and
  its per-pixel scale arithmetic: one `naviveylin::TypeIconArgbWord(...)` call per pixel into the `jint`
  vector that is handed to `SetIntArrayRegion`, and `naviveylin::TypeIconFitScale(side, extent)` in both
  drawing helpers. Verify: the observable result is identical — one `0xAARRGGBB` word per pixel with straight
  channels, `0x00000000` where nothing was painted — and no copy of the fit fraction is left in the JNI file.
- [x] 1.6 Add the requested size to the three stage warnings that the reference implementation logged without
  it (`no-style-config`, `no-type-config`, `unknown-type`), because the spec delta requires every stage
  warning to carry the stage, the type and the requested size. Verify: all five stage warnings read
  `[JNI] renderTypeIcon stage=<stage> type=<name> px=<size>` (plus `db=` / `symbol=` / `image=` where the
  reference had them), and the "the stylesheet gives this type no icon" case still logs nothing.

## 2. The two pure decisions on the host (spec: map-rendering)

- [x] 2.1 Create `libosmscout-client-java/src/type_icon_pixels.h` (no JNI, no libosmscout includes) in
  `namespace naviveylin`, with `TypeIconFitFraction` (0.9, the single spelling of the fit), the inline
  `TypeIconFitScale(int side, double extent)` (the factor that fills the fraction of a `side` x `side` square;
  `0.0` when `side <= 0` or there is no extent, which is the reference implementation's
  `if (!(extent > 0.0)) return false;` decision) and the inline `TypeIconArgbWord(uint8_t b, uint8_t g,
  uint8_t r, uint8_t a)` (un-premultiply when `a != 0 && a != 255`, clamp, pack `0xAARRGGBB`), with the
  header comment stating why the alpha is divided back out and that the byte order is the platform's B,G,R,A.
  Verify: the header includes nothing but `<algorithm>` and `<cstdint>`, so a host test can include it alone.
- [x] 2.2 Add `Tests/src/TypeIconPixelsTest.cpp` (Catch2) with distinct expected values: the fit scale for a
  square artwork, a wide one and a tall one (the longer side fills 0.9 of the square each time) and `0.0` for
  zero or negative extent and for `side == 0` (and a negative side); the pixel word for an opaque pixel, a
  fully transparent one (alpha 0, channels 0), a 50 %-alpha pixel (the exact word the implementation produces,
  justified in a comment, plus the two words a wrong implementation would produce) and an `a == 255` pixel
  (untouched). Verify: `ctest -R TypeIconPixelsTest` passes with everything discriminated (24 assertions in 5
  test cases).
- [x] 2.3 Register the test in both build systems, in the section that holds the other client-java helper
  tests: `Tests/CMakeLists.txt` (`osmscout_test_project(NAME TypeIconPixelsTest SOURCES
  src/TypeIconPixelsTest.cpp INCLUDES ${PROJECT_SOURCE_DIR}/libosmscout-client-java/src)`) and
  `Tests/meson.build` (the `executable(...)` + `test(...)` pair, mirroring `FramePixelLayoutTest`). Verify:
  the CMake target and the meson target both exist and both are run by their own test suite.

## 3. The Java declaration (spec: map-rendering)

- [x] 3.1 Add `public native int[] renderTypeIcon(String objectType, int sizePx);` to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`, after `projectToPixel`,
  with a Javadoc in the file's own style: the `0xAARRGGBB` layout of the returned `sizePx * sizePx` elements
  (the allocating render entry point's layout) and `null` when the type has no artwork or the artwork cannot
  be produced, the accepted `sizePx` range (1 to 512) and that the type must be known to the loaded type
  configuration, that the icon style comes from the active stylesheet and that a rule selecting on a tag needs
  an object and therefore yields no artwork, and that `setPreferSymbolIcons(boolean)` decides between the
  vector symbol and the raster icon. Verify: the declaration is compiled into the jar (`javap` on the built
  jar lists `public native int[] renderTypeIcon(java.lang.String, int)` in that position).
- [x] 3.2 Verify the JNI parity check counts the new declaration and no longer reports the function as dead
  bridge code. Verify: `bash scripts/check-jni-signatures.sh` exits 0 with `58 native declarations checked`
  (57 before) and `renderTypeIcon` absent from the dead-JNI-function list, whose five entries are unchanged.

## 4. Verification

- [x] 4.1 Build the touched CMake targets explicitly. Verify: `cmake --build build --target
  osmscout_client_java java_jar TypeIconPixelsTest` exits 0 with no warning from a touched file (the only
  warnings in the log are the configure-time `FindPango`/`FindGLIB2` author warnings, unrelated and
  pre-existing).
- [x] 4.2 Run the new host test, then the whole suite. Verify: `ctest --test-dir build -R TypeIconPixelsTest
  --output-on-failure` is `1/1 ... Passed`, and `ctest --test-dir build -j 8 -E Performance
  --output-on-failure` reports `100% tests passed out of 117`.
- [x] 4.3 Build and run the meson side of the same three things. Verify: `meson compile -C debug
  osmscout_client_java libosmscoutclientjava TypeIconPixelsTest` links the shared library, creates the jar and
  links the test, and `meson test -C debug --print-errorlogs "Check type icon pixels"` reports `Ok: 1, Fail: 0`.
- [x] 4.4 Validate the change. Verify: `openspec validate client-java-type-icon-entry-point --strict` reports
  `Change 'client-java-type-icon-entry-point' is valid` (every requirement description stays below strict
  mode's 500-character limit; the detail lives in this directory's `design.md`).
- [x] 4.5 Run the JavaScout suite against the freshly built jar, as a regression check of the added
  declaration. Verify: `mvn -o -f JavaScout/pom.xml test -Dnative.lib.dir=.../build/libosmscout-client-java`
  is `BUILD SUCCESS` with 230 tests, 0 failures, 0 errors.
- [ ] 4.6 Exercise the entry point end to end (a Java call that renders a real type's icon from a real
  stylesheet). NOT RUN: `maps/` holds format v26 databases while the library expects v27, so no database whose
  type configuration and stylesheet match the library can be opened here, and no such database may be
  imported in this tree. See `verification.md` section 6; the entry point's two pure decisions are covered by
  4.2 instead, and its request contract is covered by inspection.

## 5. Change hygiene

- [x] 5.1 Verify the diff touches only `libosmscout-client-java/src/OSMScoutClient.cpp`,
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`,
  `libosmscout-client-java/src/type_icon_pixels.h`, `Tests/src/TypeIconPixelsTest.cpp`,
  `Tests/CMakeLists.txt`, `Tests/meson.build` and this change's own directory. Verify: `git status --short`
  after the commit lists only the pre-existing untracked entries (`maps/repository/` and the sibling change
  directories), and no file outside the listed set appears in the commit.
- [x] 5.2 Verify `proposal.md`'s claim that the new test is registered in `Tests/CMakeLists.txt` and
  `Tests/meson.build` really holds. Verify: both files are in the commit and each registers
  `TypeIconPixelsTest` (task 2.3).
- [x] 5.3 Verify no existing function of the JNI translation unit changed and no Java-side `render` change was
  added. Verify: `git diff --numstat` on `OSMScoutClient.cpp` shows insertions only, and the Java diff is the
  one Javadoc + declaration block.
- [x] 5.4 Record the deviation from the reference implementation (task 1.6, the requested size in three stage
  warnings) so it is not silent: this file, `design.md` D6 and the pull request body name it.
