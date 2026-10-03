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
