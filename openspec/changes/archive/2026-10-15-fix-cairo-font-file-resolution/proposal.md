# Proposal

## Why

A renderer is configured with one font name, and several shipped callers configure it with a font
*file* (`Tests/src/PerformanceTest.cpp` takes `--font` and defaults it to a file path,
`Tests/src/LaneEvaluationCompare.cpp`, `Demos/src/Tiler.cpp`, `Demos/include/DrawMap.h`,
`Demos/src/TextMetricsAll.cpp`, `Demos/src/DrawMapSVG.cpp`), while the Apple apps, the Qt client, the
Java client and the font-dependent tests configure a *family* name. The Cairo backend, and the Pango
text path of the SVG backend, hand whatever they receive to interfaces that resolve a font by family,
so a file path is looked up as a family that does not exist: the caller silently paints with the
host's fallback face and only an error message naming the path hints at it. Every label those callers
render and every metric they report therefore depends on which fonts the machine happens to provide.
The SVG backend's FreeType text path is the one that already resolves a file, which is what makes the
other paths a defect rather than a missing feature.

The tests hide the defect rather than expose it: they read the family out of the font file themselves
and register it before configuring the painter, and `font-dependent-test-fonts` pins that workaround as
a requirement ("the test SHALL NOT pass the file path to the family-based interface"). The conversion
belongs to the library, so that any consumer may pass either form.

## What Changes

- The Cairo backend, in both of its build variants, and the Pango text path of the SVG backend SHALL
  serve the face contained in a configured font file, so a caller may keep configuring a file path.
- The SVG backend's FreeType text path SHALL keep the behaviour it has today.
- A configured font name that is not a file SHALL keep resolving by family, unchanged.
- A configured font name that is neither a readable file nor a resolvable family SHALL be reported and
  fall back, as today; it SHALL NOT be silently replaced by an unrelated face.
- **BREAKING** (rendering behaviour, no API change): callers that pass a font file change from the
  host's fallback face to the intended face. Their rendered labels and reported text metrics change,
  and the assertions of the tests that measure text may have to move with them.
- The tests SHALL stop converting a font file to a family name themselves once the library performs
  the conversion; the reason the workaround exists is gone, and it is gone for both backends.
- Non-goal: nothing changes for callers that configure a family name.

## Capabilities

### New Capabilities

None. The behaviour belongs to a capability the project already declares.

### Modified Capabilities

- `font-management`: this capability currently describes only the Skia backend's typeface loading and
  caching, while the map backends that resolve a font by family have no requirement at all. It gains
  the requirement for how a configured font name is turned into the face a family-resolving backend
  draws with - including that a font file is served from that file - and its purpose covers the
  family-resolving backends it now owns.
- `font-dependent-test-fonts`: the requirement "A font file argument is resolved to a family name for
  family-based backends" is retired. With the library accepting a font file, a test driver has no
  reason to convert it, and the requirement's scenarios (the Cairo driver receives a family, the
  file-based drivers keep the path) describe a distinction that stops existing. The remaining
  requirements of the capability - which font the tests measure and that their results do not move
  with the host font set - stay.

## Impact

Affected code and modules:

- `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` - `GetFont`, in both of its build
  variants, is where a configured name becomes a face; the font cache key of the same file is part of
  the contract there.
- `libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h` - the font cache and its key.
- `libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp` - the Pango variant of `Layout` resolves
  the same configured name; the FreeType variant of the same file already resolves a file and is the
  in-repo precedent the other paths follow.
- `libosmscout-map/include/osmscoutmap/MapParameter.h` - the contract of `SetFontName`/`GetFontName`
  is what callers read to decide which form to pass; it must state both forms.
- `libosmscout-map-cairo/CMakeLists.txt`, `libosmscout-map-cairo/meson.build` - the Cairo backend
  finds neither the font library nor the font configuration it would need for this, and the two build
  systems have to gain the same optional dependency, guarded so that a platform without it keeps the
  current behaviour.
- `Demos/src/TextMetricsAll.cpp` - the only family reader in the repository sits in this demo; a
  library-side reader has to exist before the demo and the tests can drop theirs.
- `Tests/include/TestFontSupport.h`, `Tests/src/PerformanceTest.cpp`,
  `Tests/src/MapPainterShieldTest.cpp`, `Tests/src/TextMetricsCairoTest.cpp`,
  `Tests/src/TextMetricsSVGTest.cpp`, `Tests/src/LaneEvaluationCompare.cpp` - the conversion and the
  drivers that perform it.
- `openspec/specs/font-management`, `openspec/specs/font-dependent-test-fonts` - the two spec deltas
  above.

Consumers that keep configuring a family name and are not expected to change: the Apple apps
(`Apple/OSMScoutOSX/OSMScoutOSX/OSMScout.mm`, `Demos/src/DrawMapOSX.mm`, `Demos/src/DrawMapAllOSX.mm`),
the Qt client (`libosmscout-client-qt/src/osmscoutclientqt/TiledMapRenderer.cpp`,
`PlaneMapRenderer.cpp`, `QmlSettings.cpp`), `libosmscout-client-java/src/OSMScoutClient.cpp` and
`libosmscout-client/src/osmscoutclient/Settings.cpp`.

Dependencies: the font family resolution the repository already uses elsewhere is already found by
the SVG backend (`libosmscout-map-svg/CMakeLists.txt`, `libosmscout-map-svg/meson.build`) and becomes
an optional dependency of the Cairo backend, which finds it today in neither build system. It stays
optional in both: a platform without it keeps the current behaviour.

Reaching both backends: the resolution has to be available to the Cairo backend and to the Pango
variant of the SVG backend, while the resolution the SVG FreeType variant performs today stays where
it is. Whether that is one shared place or one per backend is a question for the design, not for this
proposal. The backlog entry this change answers is "Cairo backend treats a font file path as a font
family name" in `TODO.md`, whose note already records that the library behaviour was left alone by
`msys-ci-font-environment` on purpose.
