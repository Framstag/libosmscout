# Verification

Evidence for `fix-cairo-font-file-resolution`: what was run, what it showed, and what could not be
verified on this machine.

## The defect, reproduced

The behaviour before the change is reproduced by the Pango-less Cairo configuration, which still
handed the configured name to the toy font API (the Pango variant behaved the same way, that is what
the change fixes):

```
build-nopango$ ctest -R TextMetricsCairoTest -V
  TextMetricsCairoTest.cpp:235: FAILED:
    REQUIRE( differs )
  with message:
    label width for the configured file: 218.154, for the substituted family: 218.154
```

The label measured with the configured **font file** had exactly the width of the face the host
substitutes for a name it cannot resolve: the file was ignored. This is also the discrimination proof
for the new case - it fails on the old code and passes on the new code.

## After the change

Measured with a one-off probe linking both backends against the built libraries. The reproducible
form of the same measurement is the new test case of either backend, which asserts it per glyph and
per label:

| backend | configured name | label width |
|---|---|---|
| Cairo | the repository font file | 220.000 |
| Cairo | a family no host resolves | 218.000 |
| SVG | the repository font file | 220.000 |
| SVG | a family no host resolves | 218.000 |

Both backends now measure the face of the configured file, distinctly from the substituted face. The
difference is small in width (2 px) because the substitution on this host is metric-compatible with
the repository font; this is why the test cases assert the per-glyph **ink boxes** against the FreeType
reference in addition, where the difference is larger and which fails on the old code
(`TextMetricsSVGTest.cpp:225` with the family assignment restored to the configured name).

No tolerance of `text-metrics-api` had to be widened: `TextMetricsCairoTest` (5 cases, 152
assertions), `TextMetricsSVGTest` (5 cases, 215 assertions) and `MapPainterShieldTest` pass with their
existing margins, and the comparison against the FreeType reference is now satisfied without any test
side preparation.

## What was run

| verification | result |
|---|---|
| `cmake --build build` | compiles; no warning from a file this change touches (the warnings reported are the pre-existing ones in `MapPainterOpenGL.cpp` and `MapWidget.cpp` that `TODO.md` records) |
| `meson compile -C build-meson` | compiles |
| `ctest -j 4 --output-on-failure` | 144 of 144 passed |
| `meson test --timeout-multiplier 2 -C build-meson` | 144 of 144 OK |
| `ctest -R 'MapPainter\|TextMetrics\|StyleConfig\|Symbol'` | 20 of 20 passed (no other backend regressed) |
| `TextMetricsCairoTest` / `TextMetricsSVGTest` / `FontNameResolutionTest` in a Pango-less configuration, Debug + AddressSanitizer + UndefinedBehaviorSanitizer | pass; no use-after-free, double-free or invalid free |
| `build-asan` (`AGENTS.md` configuration), `ctest --exclude-regex PerformanceTest` | 98 of 101 passed; the three failures are `MapPainterShieldTest`, `TextMetricsCairoTest`, `TextMetricsSVGTest` with LeakSanitizer reports, the pre-existing fontconfig/pango class `TODO.md` records |
| `PerformanceTest --driver cairo` with the repository font file over `maps/Dortmund` | completes |
| `PerformanceTest --driver opengl` with the repository font file over `maps/Dortmund` (`stylesheets/boundaries.oss`) | completes, so a file based driver still receives the path |
| `PerformanceTest --driver cairo --font README.md` | exit code 1, report names `README.md` |
| GLib warnings fatal (`G_DEBUG=fatal-warnings`) over the Cairo text tests and `PerformanceTest --driver cairo` | no pango warning: the layout created on the painter's own font map is accepted by the cairo target |

## The mechanism, probed

- `pango_font_map_add_font_file` accepts the repository font file (`add_font_file: 1`).
- The addition is confined to the font map it is given: after the call the file is **not** in the
  application font set of `FcConfigGetCurrent()`. This is what keeps the change from mutating the font
  configuration of the process, and it is why the Pango < 1.56 fallback (which does use the process
  configuration) is described as the less local one.
- A layout created with `pango_layout_new(context)` on that map and bound with
  `pango_cairo_update_layout(draw, layout)` works with warnings fatal, so the measurement and the
  drawing of one painter share one font map and one face.

## Residuals and what could not be verified here

- **The repository font's family is installed on this host** (`fc-match "Liberation Sans"` resolves),
  so the spec scenario "a host that does not provide the file as an installed family" could not be
  reproduced with the repository font. What is verified instead is the discriminating property (the
  configured file is measured differently from the substituted family) plus the probe above showing the
  file is added to the map. A host without that family is the configuration the CI does not currently
  provide for this backend.
- **The Pango < 1.56 fallback branch is compiled but not executed**: this host has Pango 1.58, and the
  fallback needs an older Pango. It calls `FcConfigAppFontAddFile`, whose effect is process wide.
- **The no-op branch of `FontNameResolution` (a build without FreeType) is not exercised by a real
  configure**, because `cmake/FindPango.cmake:112` makes FreeType a hard requirement whenever pangoft2
  is present (recorded in `TODO.md`). It was verified by compiling the unit with the feature macro
  undefined and running its assertions (`no-op branch verified`).
- **The per-painter registration adds to the pre-existing pango/fontconfig leak**: of the 1,558,992
  bytes LeakSanitizer reports for the three tests, 19,369 bytes in 70 traces pass through
  `AddFontFileToMap`; every allocation frame below it is inside `libpangoft2`/`libfontconfig`, and no
  leak is allocated by this project's own code. The tests called the same API once per process before
  this change, so the class is not new; the change makes it one registration per painter instead. The
  alternative - registering into the default font map once per process - would avoid the growth at the
  cost of the process-wide mutation the design set out to avoid.
- **The OpenGL area path has a pre-existing crash on this database** (`TODO.md`), which is why the
  OpenGL check above uses a stylesheet that loads no areas.

## The CI failure this change surfaced: the header of the linked libpng

The first CI run of the change failed the `cmake` job of the OS X workflow at
`Tests/src/MapPainterCairoPatternTest.cpp:437` (`Cairo stays quiet when it can serve the pattern`,
expansion `0 == 1`), the same case and the same shape as the failure `fix-pattern-path-wiring` had
fixed before this change. The test output named the mechanism:

```
libpng warning: Application built with libpng-1.4.12 but running with 1.6.58

/Users/runner/work/libosmscout/libosmscout/Tests/src/MapPainterCairoPatternTest.cpp:437: FAILED:
  REQUIRE( capture.CountLinesContaining("Loaded pattern image")==1 )
with expansion:
  0 == 1
```

`LoaderPNG` passes `PNG_LIBPNG_VER_STRING` to `png_create_read_struct`, and libpng returns `NULL` when
that string does not match the library it runs in, so the shipped pattern was not decoded and the
backend reported a pattern it could serve as unloadable. The version strings say which side is wrong:
the header is an old one (1.4.12), the library is the pinned Homebrew keg (1.6.58), and the configure
output of the job shows the only directory of that host that holds such a header:

```
-- Found PNG: /opt/homebrew/opt/libpng/lib/libpng.dylib (found version "1.6.58")
-- Found Pangoft2: ...;/Library/Frameworks/Mono.framework/Headers;/opt/homebrew/include/freetype2
```

The pin of `PNG_PNG_INCLUDE_DIR` and `PNG_LIBRARY` that `fix-pattern-path-wiring` added is in place and
does what it should - `PNG::PNG` names the keg - but it only decides what `PNG::PNG` carries, not what
the compiler searches first. `IMPORTED` targets contribute their include directories as *system*
include directories, and a compiler searches system directories only after the include directories of
the target itself, so any other directory that reaches the compile line before them wins: on the
runner that is the `Headers` directory of the Mono framework (a `png.h` of 1.4.12 next to fontconfig
and FreeType headers), which this change put on the compile line of `libosmscout-map-cairo` by linking
`Fontconfig::Fontconfig` and `Freetype::Freetype` for the font support, and which `OSMScoutMap` now
exports to every consumer for the same reason. The meson job of the same run passed the same test,
because meson passes every dependency as a plain include directory and its order puts the libpng
header first - which is also why no meson change was needed.

`cmake/features.cmake` therefore asks for no system treatment of `PNG::PNG`
(`IMPORTED_NO_SYSTEM`), so that the header of the libpng a library links sits in the group of
include directories the compiler searches first, whatever else a host puts on the include path. The
change is one property, next to the `find_package(PNG)` that creates the target, and it holds for
every consumer of `PNG::PNG` in this project (`libosmscout-map-cairo`, `libosmscout-map-opengl` and
the two demos that include `png.h`), so the next dependency that arrives with a stray `png.h` on its
include path cannot repeat this.

Verified here, with the flag order CMake generates for the `libosmscout-map-cairo` unity compile in a
Cairo + Pango build. `PNG_PNG_INCLUDE_DIR` is pointed at a stand-in directory that carries a `png.h`
(the stand-in for the Mono framework's `Headers` directory, where the runner has both the `png.h` and
the `fontconfig/fontconfig.h`), and the fontconfig include directory is set to that same directory:

| | flags of the dependencies that carry a `png.h` |
|---|---|
| before | `-isystem /usr/include/freetype2`, `-isystem <stand-in>/png.h`, `-isystem <stand-in>/fontconfig`, `-isystem /usr/include/harfbuzz` - the stand-in directory comes after the FreeType and PNG directories of the system group |
| after | `-I<stand-in>/png.h` first, then the same `-isystem` directories without it - the compiler searches the `-I` group before every system directory, so the header of the linked libpng wins whatever the order |

The `/usr/include` that `PNG_INCLUDE_DIRS` also carries (the ZLIB directory) is still dropped by CMake
as an implicit compiler directory - the real configure and the full build on this machine are
unchanged by the property - so no other target of the build sees a new include directory.

What could not be verified here: the macOS runner and its Mono framework. The fix was derived from
the CI logs of this change and from the mechanism of the failure `fix-pattern-path-wiring` fixed with
a pin, and its effect on the flag order was verified on this machine; the OS X job has to confirm it.

