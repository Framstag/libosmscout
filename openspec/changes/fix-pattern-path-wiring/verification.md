# Verification: fix-pattern-path-wiring

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- `build/` — CMake, Ninja, `CMAKE_BUILD_TYPE=Release`, Cairo/Qt/Skia backends enabled, `OSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF`.
- `build-meson/` — Meson build; its configuration enables `OSMScoutOpenGL` and the Qt/Skia tests, so it compiles the file the CMake configuration skips.
- Demo runs use `Demos/DrawMapCairo` with `maps/Dortmund` at magnification factor 32768 (level 15) and `stylesheets/standard.oss`; the font comes from `fc-match "Liberation Sans"`.
- Note for the demo runs: the positional `zoom` of the demos is a magnification *factor* (`Magnification::SetMagnification`), not a level, so 32768 is level 15. Runs made before this was noticed loaded no data (`Data: 0+0 0 0`) and could not have shown a pattern.

## 1.1 Shared verdict helper

`libosmscout-map/include/osmscoutmap/PatternLookup.h` and `src/osmscoutmap/PatternLookup.cpp` add `PatternLookup::Status` (`Found`, `NotFound`, `NoSourceConfigured`), `Resolve()` (returns the file that holds the image and joins a directory without a trailing separator) and `Describe()` (the report text). Registered in `libosmscout-map/CMakeLists.txt`, `libosmscout-map/src/meson.build` and `libosmscout-map/include/meson.build`.

`cmake --build build --target OSMScoutMap` compiles it; the full build and `meson compile -C build-meson` link it. clang-tidy on the new source reports one advisory (`bugprone-easily-swappable-parameters`, also 13 times in the untouched `MapParameter.cpp`) and the build-system/pre-existing-include diagnostics every file carries; the direct-include and enum-undersize findings it reported were fixed.

## 1.2 Unit test of the helper

`Tests/src/PatternLookupTest.cpp`, five cases: no directory configured, directories searched without the image, image found in the second directory, directory without a trailing separator, and a non-matching extension.

```
ctest -R PatternLookupTest --output-on-failure          -> Passed
meson test -C build-meson "Check pattern image lookup"  -> OK
```

## 2.1 Cairo report

`MapPainterCairo::HasPattern` now asks `PatternLookup::Resolve` for the file, loads it when found, and otherwise reports `PatternLookup::Describe(...)`; the `GetPatternId()==0` marker still short-circuits the next draw, so the report stays one per pattern. A found-but-unloadable file keeps its own `ERROR while loading pattern image '<file>'` message.

## 2.2 Cairo render test

`Tests/src/MapPainterCairoPatternTest.cpp` renders an area whose fill style uses a pattern into an in-memory surface with a capturing logger installed through `Log::SetLogger`:

- no pattern image source: exactly one `No pattern image source is configured` line naming `natural_scrub`, and the surface carries the solid fill colour;
- two areas with the same pattern: still exactly one report;
- shipped image directory configured: one `Loaded pattern image` line (debug level enabled for the case), no misconfiguration line, and the fill style is not left marked as an unresolvable pattern.

```
ctest -R MapPainterCairoPatternTest --output-on-failure  -> Passed (3 cases, 25 assertions)
```

## 2.3 Qt, Skia, IOS

- Qt: `MapPainterQt::HasPattern` resolves through the helper (extension `.svg` for `PatternMode::Scalable`, otherwise `.png`), reports `Describe(...)` at error level when it cannot serve the pattern, and keeps `Cannot load pattern '<name>' from "<file>"` for a found-but-unloadable file.
- Skia: the pattern block in `MapPainterSkia::DrawArea` resolves once through the helper instead of probing every path per draw, caches the shader by the resolved file, and reports the misconfiguration where it previously fell back silently; the failure marker (now `GetPatternId()==0`) makes the report one per pattern.
- IOS: `MapPainterIOS::HasPattern` mirrors the Cairo path with the `@Nx` extension of its content scale and now marks a failed pattern with `0` instead of `SIZE_MAX`, which is what makes the existing `GetPatternId()==0` guard report once; **not built or run on this machine** (Apple-only), see the limitation below.

```
meson test -C build-meson "Check MapPainterSkia compilation"      -> OK
QT_QPA_PLATFORM=offscreen meson test -C build-meson "Check MapPainterShieldQt compilation" \
                                                   "Check TextMetricsQt compilation"       -> OK
ctest -R "MapPainterSkiaTest|TextMetricsCairoTest|SymbolRendererCairoTest" -> Passed
```

## 2.4 Backends build

```
cmake --build build --target OSMScoutMapCairo OSMScoutMapQt OSMScoutMapSkia   -> no warnings in the touched files
meson compile -C build-meson                                                 -> success
```

## 3.1 Demos configuration

`Demos/include/DrawMap.h` gains a `patternPath` option (visible in `DrawMapCairo --help`), uses the icon directories as the default pattern source and `SetPatternPaths` next to `SetIconPaths`.

```
DrawMapCairo --debug --database maps/Dortmund --iconPath libosmscout/data/icons/14x14/standard \
             --width 1600 --height 1600 stylesheets/standard.oss 51.5214 7.5127 32768 out.png
  -> Loaded pattern image 'libosmscout/data/icons/14x14/standard/natural_scrub.png'
     Loaded pattern image 'libosmscout/data/icons/14x14/standard/leisure_garden.png'
     Loaded pattern image 'libosmscout/data/icons/14x14/standard/landuse_cemetery.png'
     (same three lines at 51.5045 7.4746 and 51.4786 7.4649)

... --patternPath libosmscout/data/icons/svg/standard      (no PNG in it)
  -> Pattern image 'landuse_cemetery.png' not found in the configured pattern directories '<dir>'
     (one line per pattern; same for leisure_garden.png and natural_scrub.png)

... without --iconPath and --patternPath
  -> No pattern image source is configured, pattern 'landuse_cemetery' cannot be resolved
     (3 lines, one per pattern)
```

Before the change the two failure runs produced `ERROR while loading pattern image '<name>'` without saying that nothing had been searched, and the successful run loaded no pattern at all.

## 3.2 OSMScoutOpenGL configuration

`OSMScoutOpenGL/src/OSMScoutOpenGL.cpp` passes the directory list it already builds to `SetIconPaths` and `SetPatternPaths`. The tool is disabled in the CMake configuration used here, but `meson compile -C build-meson OSMScoutOpenGL` compiles and links it. **No runtime render was performed**: the tool opens a GLFW window and needs a display plus `--shaders`/`--font`, which is not available in this environment.

## 3.3 Entry points that already configured both lists

Read and confirmed unchanged: `libosmscout-client-qt/src/osmscoutclientqt/IconLookup.cpp:47-48`, `TiledMapRenderer.cpp:490-491`, `PlaneMapRenderer.cpp:352-353`, `Apple/OSMScoutOSX/OSMScoutOSX/OSMScout.mm:54-55`, `Android/OsmScoutViewer/.../OsmScoutViewerActivity.java:116` and `Android/OsmScoutBenchmark/.../OsmScoutBenchmarkActivity.java:149` (both call `mMapParameter.setPatternPaths(iconPaths)` over JNI).

## 4.1 Shipped pattern references are checked

`Tests/src/StyleConfigSymbolsTest.cpp` gains a case that loads every `.oss` file in `stylesheets/` (nine shipped stylesheets), collects `GetPatternNames()` and resolves each name against `libosmscout/data/icons/14x14/standard` with `PatternLookup::Resolve`. It fails naming the pattern and the stylesheet, and requires that it saw at least one pattern so it cannot pass vacuously.

```
ctest -R StyleConfigSymbolsTest          -> Passed (5 cases)
meson test -C build-meson "Check StyleConfig symbol enumeration"  -> OK
```

## 4.2 The check fails when it should

With the referenced images moved aside, the check reports nine references and names them, for example:

```
unresolved pattern references: cycle.oss: landuse_cemetery; cycle.oss: natural_scrub;
cycle.oss: leisure_garden; standard.oss: landuse_cemetery; standard.oss: leisure_garden;
standard.oss: natural_scrub; winter-sports.oss: landuse_cemetery; winter-sports.oss:
leisure_garden; winter-sports.oss: natural_scrub;
```

Removing only `leisure_garden.png` fails the check as well and names that pattern with the stylesheets that
reference it (the captured message showed `standard.oss: leisure_garden` and `winter-sports.oss:
leisure_garden`; the probe with all three images moved aside above lists every reference, including the
three stylesheets that use `leisure_garden`). All images were restored afterwards (`git status` clean for
`libosmscout/data/icons/`), and the check passes again.

## 4.3 No missing image

The check found no unresolved reference with the shipped images, so no image had to be added and no style had to be disabled.

## 5.1 Full builds and suites

```
cmake --build build                                                     -> success
CTest -j 2 --exclude-regex "PerformanceTest" (QT_QPA_PLATFORM=offscreen) -> 101/101 passed
meson compile -C build-meson                                            -> success
meson test --timeout-multiplier 2 (QT_QPA_PLATFORM=offscreen)            -> 140/140 OK
```

Rebuilding every touched file produced no warning of its own; the warnings in the build belong to pre-existing entries in `TODO.md` (`GenTextIndex.cpp:188`, `MapPainterOpenGL.cpp:464`, the Qt client deprecations, the vendored `nanosvg.h`) and to the Doxygen configuration.

## 5.2 TODO.md and 5.3 AGENTS.md

- `TODO.md`: the pattern-wiring entry is annotated as closed by this change (the report, the wiring, the IOS part and the verification are recorded in it), and the symbol/pattern gate entry states that the pattern half is now gated while the symbol half is not.
- `AGENTS.md`: the "Common Patterns" list records that a pattern image is an icon image, that an entry point passes its image directories to both `SetIconPaths` and `SetPatternPaths`, and that `StyleConfigSymbolsTest` resolves the shipped pattern references.

## Open limitations

- **IOS backend not verified**: `MapPainterIOS.mm` is Apple-only; the change mirrors the Cairo path but neither compiles nor runs in this environment. It needs the iOS/macOS workflows.
- **OSMScoutOpenGL render not verified**: compile-verified only; a render needs a display, shaders and a font.
- **Uncrustify and clang-tidy are not usable gates** (pre-existing, `TODO.md` entry "Neither formatting nor static analysis is currently a usable gate"): Uncrustify 0.83.0 reports a difference for every file tried, including untouched ones, so a clean whole-tree run is not achievable. The new `PatternLookup` files and the two new test files are clean under the repository config; the pre-existing drift of the files this change edited was left alone.
