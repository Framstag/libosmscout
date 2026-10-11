# Proposal

## Why

The shipped stylesheets now use pattern fills in roughly 25 places, but the two reference entry points in
this repository (the demo tools and `OSMScoutOpenGL`) configure only the icon image directories. Every
pattern-filled area therefore renders as its flat fallback colour, and the only trace is a single log
line: the failure is silent to whoever runs the demo and invisible to the build and the test suite. The
consumers that do configure both directories (Qt client, Apple, Android) show the pattern, so the defect
is an inconsistency between entry points rather than a rendering gap.

## What Changes

- Every in-repo entry point that renders a stylesheet configures the pattern image directories wherever it
  configures the icon image directories; the demo tools and `OSMScoutOpenGL` join the Qt client, Apple and
  Android.
- A render that consumes a stylesheet using pattern fills while no usable pattern directory is configured
  reports that condition once, naming the stylesheet's pattern usage and the directories that were
  searched, instead of silently drawing the solid fallback.
- A test asserts that every pattern referenced by the shipped stylesheets resolves to an image in the
  shipped pattern directory, so a missing image or an unwired directory fails a test instead of a render.
- Entry points that already configure both directories keep their behaviour; the warning path is the only
  new runtime output.

## Capabilities

### New Capabilities

- `pattern-path-configuration`: the contract for rendering a stylesheet that uses pattern fills - that an
  entry point which renders such a stylesheet provides the pattern image directories, that a render without
  a resolvable pattern source is reported rather than silently degraded, and that the shipped stylesheets'
  pattern references resolve to shipped images.

### Modified Capabilities

None.

## Impact

- `Demos/include/DrawMap.h` - the demo tools' render parameter setup and its argument handling; a pattern
  directory option next to the existing icon directory option.
- `OSMScoutOpenGL/src/OSMScoutOpenGL.cpp` - the OpenGL tool's render parameter setup.
- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` and
  `libosmscout-map/include/osmscoutmap/MapParameter.h` - the reporting point for a pattern fill that no
  configured directory can serve (`MapParameter` already carries the pattern directory list).
- `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp`,
  `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp`,
  `libosmscout-map-agg/src/osmscoutmapagg/MapPainterAgg.cpp`,
  `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` - the per-backend pattern lookup that today
  logs a lookup failure without stating that no directory was configured.
- `Tests/src/StyleConfigSymbolsTest.cpp`, `Tests/CMakeLists.txt`, `Tests/meson.build` - the test that
  resolves every referenced pattern name against the shipped directory, registered in both build systems.
- `stylesheets/include/*.oss` and `libosmscout/data/icons/**` - read only, as the inputs the test checks;
  changed only if a referenced pattern image turns out to be missing.
- `AGENTS.md` - only if the pattern directory convention needs recording next to the icon directory one.
