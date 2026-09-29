# Tasks

## 1. Pattern lookup verdict (spec: pattern-path-configuration - "A pattern fill that cannot be served is reported")

- [x] 1.1 Add the shared verdict helper to `libosmscout-map` (new header/source pair under `include/osmscoutmap/` and `src/osmscoutmap/`) that classifies a pattern lookup outcome - no pattern image source configured, no image found in the given directories, found - and names the directories searched; verify it compiles via `cmake --build build --target osmscoutmap`.
- [x] 1.2 Add `Tests/src/PatternLookupTest.cpp` (Catch2) covering the three outcomes, including the empty-directory-list case and a directory list where only the second entry holds the image; register it in `Tests/CMakeLists.txt` (`osmscout_test_project`, next to `StyleConfigSymbolsTest`) and in `Tests/meson.build` with the same environment; verify `ctest -R PatternLookupTest --output-on-failure` and `meson test -C build-meson "Check pattern image lookup"` pass.

## 2. Backend reporting (spec: pattern-path-configuration - "A pattern fill that cannot be served is reported")

- [x] 2.1 Use the verdict helper in `MapPainterCairo::HasPattern` (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:534`) so the empty-list case reports the misconfiguration and a failed lookup names the pattern and the directories, keeping the once-per-pattern behaviour of the `GetPatternId()==0` marker.
- [x] 2.2 Add a Catch2 test that installs a capturing logger with `Log::SetLogger`, renders an area whose style uses a pattern with no pattern paths configured, and asserts the reported misconfiguration and that the solid fallback colour is used; verify `ctest -R MapPainterCairoPatternTest` passes.
- [x] 2.3 Apply the same verdict and report to the other pattern-capable backends - Qt (`MapPainterQt.cpp:169`), Skia (`DrawArea` in `MapPainterSkia.cpp`, which reports nothing today) and IOS (`MapPainterIOS.mm:206`); verify each call site by running its existing rendering test (`TextMetricsQtTest`/`SymbolRendererSkiaTest` where present) and by confirming the same message text is produced from the same helper; note in `design.md` if a backend cannot be covered by a test on this machine (IOS needs an Apple run).
- [x] 2.4 Verify the four backends still build - `cmake --build build` for Cairo/Qt/Skia and `meson compile -C build-meson` - with no new warnings in the touched files.

## 3. Entry point wiring (spec: pattern-path-configuration - "Rendering entry points configure pattern image sources")

- [x] 3.1 In `Demos/include/DrawMap.h` add a `patternPath` option next to `iconPath` (`:159`), resolve the pattern directories (explicit options, else the icon directories, documented in the option help) and pass them with `SetPatternPaths` next to `SetIconPaths` (`:357`); verify by running a demo renderer over `maps/Dortmund` at detail zoom and observing a pattern-filled area (its log line now reads `Loaded pattern image '<name>'`).
- [x] 3.2 In `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp` pass the directory list it already builds (`:305-307`) to both setters; verify with one render run using `--shaders libosmscout-map-opengl/data/shaders` and a `--font` that exists on the machine, confirming the pattern log line and no regression in the drawn map.
- [x] 3.3 Confirm the entry points that already configure both lists are unaffected by reading their call sites (`IconLookup.cpp:47-48`, `TiledMapRenderer.cpp:490-491`, `PlaneMapRenderer.cpp:352-353`, `OSMScout.mm:54-55`, both Android activities) and record the confirmation in the change's verification notes.

## 4. Shipped pattern references (spec: pattern-path-configuration - "Shipped pattern references resolve to shipped images")

- [x] 4.1 Extend `Tests/src/StyleConfigSymbolsTest.cpp` to resolve every pattern name the shipped stylesheets use against the shipped image directory and fail naming the pattern and stylesheet when one does not resolve; verify the test passes on the current tree with `ctest -R StyleConfigSymbolsTest` and `meson test -C build-meson "Check StyleConfig symbol enumeration"`.
- [x] 4.2 Verify the check fails when it should: temporarily rename one referenced pattern image, confirm the test fails with the pattern name and stylesheet in the message, then restore the file.
- [x] 4.3 Add any pattern image the check proves missing to `libosmscout/data/icons/14x14/standard/`, or record in `design.md` why the referencing style is disabled instead; verify the check passes after the change.

## 5. Integration and bookkeeping

- [x] 5.1 Build both build systems without errors - `cmake --build build` and `meson compile -C build-meson` - and run the suites (CMake `ctest -j 2 --output-on-failure --exclude-regex PerformanceTest`, Meson `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs`); report the results in the change's verification notes and confirm no test regressed.
- [x] 5.2 Update `TODO.md`: annotate the pattern wiring entry as closed by this change (keeping the entry until the change is archived, as the other closed entries do) and record the shipped-pattern check so the symbol-consistency entry states what still has no gate.
- [x] 5.3 Update `AGENTS.md` if the pattern directory convention (an image directory serves both icons and patterns) needs to stand next to the icon directory note; verify the added text names the entry points that must pass both lists.
