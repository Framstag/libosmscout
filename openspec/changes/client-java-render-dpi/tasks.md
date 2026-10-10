# Tasks

## 1. Native render entries take the DPI (spec: map-rendering)

- [x] 1.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, add the DPI parameter to both native render
  entry points and pass it from the base `render` entry point into the overlay entry point. Verified: the
  JNI function names are unchanged (`Java_com_framstag_libosmscout_client_OSMScoutClient_render`,
  `…_renderWithRouteAndPois`), so no re-declaration is needed; `render` forwards the `dpi` it received.
- [x] 1.2 In the overlay entry point's render body, project with the request's DPI; when the request states
  an undefined or non-positive DPI, fall back to the DPI configured on the client and do not reject the
  frame. Verified: `if (!(dpi > 0.0)) { dpi = data->settings ? data->settings->GetMapDPI() : 96.0; }` is
  the only place the render body reads the configured value, and no path returns early for an unusable DPI.
- [x] 1.3 Remove the client-wide DPI setter (the JNI function and its comment block). Verified:
  `git grep -n setMapDpi libosmscout-client-java` finds nothing, and `scripts/check-jni-signatures.sh`
  no longer lists it among the dead JNI functions.
- [x] 1.4 Verify the DPI stays a property of the request: nothing in the render path writes the configured
  settings value. Verified: the Java-level probe rendered at 400 dpi and then without a DPI: the later
  frame is pixel-identical to the 96-dpi frame (`NO_DPI_STILL_96_AFTER_400=true`), so the 400-dpi request
  left the configured value alone.

## 2. Java declarations (spec: map-rendering)

- [x] 2.1 Declare the DPI parameter on the native `render` and `renderWithRouteAndPois` declarations in
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`, with Javadoc stating
  that an undefined or non-positive value renders with the DPI configured on the client.
- [x] 2.2 Add the non-native forwarders with the previous argument lists that pass "no DPI", so existing
  callers and tests keep compiling. Verified: no Java call site in the repository had to change —
  `JavaScout`'s `MapRenderer` (both call sites) and the JavaScout tests still call the 6-argument and
  14-argument forms, and `renderWithRoute()` forwards through the same overload.
- [x] 2.3 Update the Javadoc cross references that still name the old argument count. Verified: the three
  links that named a non-existent overload (`#render` in `setBasemapLookupDirectory`,
  `renderWithRouteAndPois`, and the `renderWithRoute` convenience link) now name the declared lists.
- [x] 2.4 Run `scripts/check-jni-signatures.sh` and verify it passes. Verified: 55 native declarations
  checked, 5 dead JNI functions reported (was 6 — `setMapDpi` is gone), exit code 0.

## 3. Verification (spec: map-rendering)

- [x] 3.1 Build the CMake build with the Java client library and the jar, and verify it compiles without
  errors and without warnings from the touched files. Verified:
  `cmake --build build --target osmscout_client_java java_jar` rc=0, no warning from `OSMScoutClient.cpp`.
- [x] 3.2 Build the Meson build the same way, including the `check-jni-signatures` test, and verify it
  compiles and the signature test passes. Verified:
  `meson compile -C build-meson osmscout_client_java libosmscoutclientjava` rc=0 and
  `meson test -C build-meson "Check JNI signature parity"` → 1/1 OK.
- [ ] 3.3 Verify the frame contract on a real map: a request with an explicit DPI produces a frame projected
  at that DPI, and a request without one produces the configured-DPI frame. Verify: with a routable or
  renderable map directory the Java test reports both frames, and reports a skip without one. This is the
  only feasible level, because the DPI plumbing needs a database and a JNI environment; there is no host
  unit test for it.
  Not run as a repository test — this change adds no Java-level DPI test (the proposal's Impact section
  names no test file). The contract was instead observed with a throw-away Java probe over a real JNI
  client (see `verification.md` §3): the 400-dpi frame differs from the 96-dpi frame in 1400 of 3072 pixels,
  and the NaN / negative / omitted forms are pixel-identical to it. `maps/` holds a routable database
  (`arnsberg-regbez`) whose `types.dat` is format version 26 while the tree expects 27, so it cannot be
  opened; the probe used the repository fixture `Tests/data/testregion` instead.
- [x] 3.4 Run the Java client suite and verify no existing test regresses. Verified: `mvn -o test` in
  `JavaScout` reports 230 tests, 19 failures, 25 skipped — the *same* failure set as a baseline run of
  `master` with this change stashed and rebuilt (see `verification.md` §4), so nothing regressed. No test
  in the repository calls the removed setter (it has no Java declaration at all).
- [ ] 3.5 Run the rest of the C++ suite and verify no existing test regresses.
  Not run: no C++ test links or reads `libosmscout-client-java` (only `JniSignatureParityTest` reads the two
  touched sources, see `Tests/CMakeLists.txt:195`), and that test passes in both build systems. Building
  every other test target to run a suite that cannot observe this change was left out deliberately.
- [x] 3.6 Run `openspec validate "client-java-render-dpi" --strict` and verify the change validates.
  Verified: exits 0.

## 4. Documentation and change hygiene

- [x] 4.1 Verify the new parameter is documented in the Java declaration and that the fallback rule is
  stated, so a caller knows what "no DPI" means. Verified: both native declarations and both forwarders
  carry the `@param dpi` text with the `Double.NaN` / non-positive fallback rule.
- [x] 4.2 Verify the removal of the client-wide setter is recorded in the change. Verified, with a
  correction to the first draft: the removal is *not* a Java source break — on master `setMapDpi` is only a
  JNI function, with no declaration in `OSMScoutClient.java` and no Java caller (TODO.md's JNI backlog entry
  lists it among six such functions). `proposal.md` and `design.md` (D3, Risks, Migration Plan) were
  corrected accordingly; nothing has to be named as a source break in the release notes.
- [x] 4.3 Verify the diff touches only the files listed in the proposal's Impact section, and that no map,
  database or style file changed. Verified: `libosmscout-client-java/src/OSMScoutClient.cpp` and
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`, plus this change
  directory.
