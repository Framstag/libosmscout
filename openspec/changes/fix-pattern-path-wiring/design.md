# Design

## Context

See `proposal.md` - Why. The current state that shapes the approach:

- Pattern fills are drawn inside each backend's `DrawArea`; the core painter only resolves the `FillStyle`
  per ring (`MapPainter::PrepareAreaRing`, `libosmscout-map/src/osmscoutmap/MapPainter.cpp:1305-1395`) and
  never loads an image. There is no pattern helper in `libosmscout-map/include/osmscoutmap/MapPainter.h`.
- Pattern images are the same PNG files as the icons, shipped in `libosmscout/data/icons/14x14/standard/`
  (`leisure_garden.png`, `natural_scrub.png`, `landuse_cemetery.png`, ...). A pattern directory is
  therefore an icon directory, which is what the Qt client already assumes: it passes one list to both
  setters (`libosmscout-client-qt/src/osmscoutclientqt/IconLookup.cpp:47-48`).
- `MapParameter` already carries the second list: `patternPaths`
  (`libosmscout-map/include/osmscoutmap/MapParameter.h:62-63`, `SetPatternPaths` at `:133`,
  `GetPatternPaths` at `:214`). Nothing in the map library is missing here.
- Backends that fill with patterns: Cairo (`MapPainterCairo.cpp:534-587`), Qt (`MapPainterQt.cpp:169`),
  IOS (`MapPainterIOS.mm:206`), Skia (`MapPainterSkia.cpp`, `DrawArea`). AGG and SVG never touch the
  pattern API and draw the solid colour.
- Current failure shape: with an empty pattern list Cairo/Qt still log
  `ERROR while loading pattern image '<name>'` - correct verdict, wrong reason, since nothing was
  searched; Skia falls back without any report at all.
- The two repository entry points that are wrong set only the icon list
  (`Demos/include/DrawMap.h:357`, `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp:307`), and the diagnostic style
  used next to them is `std::cerr` + `WARNING:` (`DrawMap.h:360`).
- The shipped stylesheets use `pattern:` roughly 25 times; `StyleConfig::GetPatternNames()` enumerates
  them and `Tests/src/StyleConfigSymbolsTest.cpp` already parses the stylesheets and the type definition
  through `TESTS_TOP_DIR`.

## Goals / Non-Goals

**Goals:**

- Every in-repo entry point that renders a stylesheet supplies a pattern image source, and the shipped
  stylesheets are verifiable against the shipped images.
- The "pattern fill cannot be served" condition names the misconfiguration instead of the file, and is not
  silent in any backend that implements pattern fills.

**Non-Goals:**

- Implementing pattern fills in AGG or SVG (separate entry: name-based icon support in AGG).
- Changing where pattern images live, or adding a pattern-specific directory.
- Preloading or caching pattern images (separate improvement entries).
- Naming the stylesheet in the misconfiguration report: the backends see only the resolved `FillStyle` and
  the `MapParameter`, and `StyleConfig` does not keep the file it was loaded from, so naming it would need a
  new `StyleConfig` field plus a way to reach every backend's `DrawArea`. The report names the pattern and
  the sources considered instead (decided 2026-09-27).
- The symbol half of the style-consistency question; this design covers patterns only.

## Decisions

### D1 - The misconfiguration report stays in the pattern-capable backends

Chosen: each backend that fills with patterns keeps its lookup and emits the report there, distinguishing
"no pattern image source is configured" from "no image found in the configured directories"; the wording and
the verdict come from one small shared helper in `libosmscout-map` that classifies the outcome and names the
directories searched, while loading the image stays in the backend.

- Alternative A - a virtual capability in `MapPainter` plus one report in the core painter. Rejected: the
  core painter does not load images and does not know whether a backend fills with patterns, so it needs a
  new API on every backend (including AGG/SVG, which would have to answer for a feature they do not have).
- Alternative B - one shared resolver in `libosmscout-map` that loads images and returns a status (missing
  source / not found / loaded). Rejected: each backend keeps its images in its own type (`cairo_surface_t*`,
  `QImage`, `sk_sp<SkImage>`), so a shared loader would have to expose all of them, and the once-per-pattern
  state already exists in the backends (`FillStyle::GetPatternId()==0` marks a failed pattern and
  short-circuits the next call). The shared part stays a pure classifier, which also makes it unit-testable
  without a painter.

### D2 - The two entry points receive the image directory list in both lists

Chosen: `Demos/include/DrawMap.h` and `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp` pass the directories they
already resolve for icons to the pattern list as well, and the demo tool exposes a pattern directory option
next to its icon option so a caller can differ deliberately.

- Alternative A - derive the pattern list inside `MapParameter::SetIconPaths`. Rejected: it hides the
  two-list contract, cannot be turned off, and breaks a caller that intentionally uses a different set for
  patterns.
- Alternative B - ship patterns in their own directory. Rejected: the images *are* icon images; a second
  copy would have to be kept in sync and would change the shipped layout for no functional gain.

### D3 - The shipped-pattern check is a Catch2 test in the existing harness

Chosen: extend `Tests/src/StyleConfigSymbolsTest.cpp` so that every pattern name from the shipped
stylesheets is resolved against the shipped image directory, and register the test in `Tests/CMakeLists.txt`
and `Tests/meson.build`.

- Alternative A - a shell script under `scripts/`. Rejected: it would have to parse the `.oss` syntax or
  duplicate the name extraction, and it would still need a place in CI; the test target already has both.
- Alternative B - extend `Demos/src/SymbolsAll.cpp`, which already warns about unresolved patterns.
  Rejected: it is a tool that prints, not a gate that fails, and it is not part of either test suite.

### D4 - Report level and frequency

Chosen: `log.Error`, once per pattern per painter, using the existing failed-pattern marker; a second
occurrence of the same pattern is not reported again. Skia and IOS receive the same report where they lack
one today (Skia currently reports nothing).

- Alternative A - `log.Warning` on every frame that draws the pattern. Rejected: the draw path is hot and
  a pan would repeat the same line per frame.
- Alternative B - a single report at style load. Rejected: the style is loaded before the render parameter
  is known, so the configured directories cannot be part of the message there.

## Render sequence

```
 demo tool / OSMScoutOpenGL        MapPainter                 backend (Cairo/Qt/Skia/IOS)
        |                              |                              |
        | MapParameter: icon paths     |                              |
        | MapParameter: pattern paths  |  (missing today -> the fix)  |
        |----------------------------->|                              |
        |                              | DrawAreas                     |
        |                              | PrepareAreaRing -> FillStyle  |
        |                              | (pattern name, no image)      |
        |                              |----------------------------->| DrawArea
        |                              |                              |  HasPattern(parameter, style)
        |                              |                              |    patternPaths empty ?
        |                              |                              |      yes -> Error "no source configured"
        |                              |                              |              + mark pattern failed
        |                              |                              |    loop over patternPaths
        |                              |                              |      image found -> cache, fill
        |                              |                              |      none found  -> Error "<name> not found
        |                              |                              |                       in <dirs>"
        |                              |                              |              + mark pattern failed
        |                              |<-----------------------------| solid fallback colour
        |<-----------------------------|                              |
        | next draw of the same pattern -> marked failed -> no second report
```

## Risks / Trade-offs

- [Report inside the draw path] -> the once-per-pattern marker bounds it to one line per pattern; the
  empty-list case is decided from `MapParameter`, so it can also be reported the first time that style is
  drawn and never again.
- [The demos start drawing patterns they never drew] -> intended, but it changes what the demo output looks
  like; verify visually (`SymbolsAll` contact sheet and one Dortmund render at `detail` zoom) in addition to
  the automated check.
- [The new test depends on `TESTS_TOP_DIR` and on the shipped image layout] -> use the same environment
  fallback as the existing test and keep the image directory argument explicit; a missing environment
  makes the test fail loudly rather than pass vacuously.
- [A referenced pattern image turns out to be missing] -> the test forces the image to be added to
  `libosmscout/data/icons/14x14/standard/`; that is the desired outcome, but it is a change to shipped data
  and belongs in the tasks explicitly.
- [Test registered in one build system only] -> the known roster divergence; the tasks list both build
  files explicitly and the change verifies both builds.
- [The IOS backend is not compile- or test-verified on this machine] -> the Cairo, Qt and Skia paths are
  built and exercised here; the IOS change mirrors the Cairo one and is only verified by the Apple
  workflows (`build_and_test_on_ios.yml`, `build_and_test_on_osx.yml`), which cannot run in this
  environment. Recorded as an open verification step, not as verified.

## Migration Plan

None. No database or type-definition format changes, and `MapParameter` already offers the API used. Rollback
is a revert of the wiring lines, the report change and the test.

```
 render of an area whose style uses a pattern      before                    after
 ------------------------------------------------  ------------------------  ---------------------------------
 pattern image source configured, image present    pattern filled            unchanged
 pattern image source configured, image missing    Error "loading image X"   Error "X not found in <dirs>"
 no pattern image source configured                Error "loading image X"   Error "no pattern source
 (demo tools, OSMScoutOpenGL)                      (nothing was searched)     configured; stylesheet uses X"
 backend without pattern support (AGG, SVG)        solid fill, no report     unchanged (Non-Goal)
```

## Open Questions

- Whether the demo tools get a separate `--patternPath` option or a documented "the icon directory is also
  the pattern directory" default. The demo option set is not part of any shipped contract, so the tasks can
  decide it when they wire the tools.
