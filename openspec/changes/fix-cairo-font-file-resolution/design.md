# Design

## Context

See `proposal.md` - Why. What shapes the approach:

- `MapParameter::SetFontName` (`libosmscout-map/include/osmscoutmap/MapParameter.h`) carries one string,
  and shipped callers pass a family name (Apple apps, Qt client, Java client) or a font file
  (`Tests/src/PerformanceTest.cpp`, `Tests/src/LaneEvaluationCompare.cpp`, `Demos/src/Tiler.cpp`,
  `Demos/include/DrawMap.h`, `Demos/src/TextMetricsAll.cpp`, `Demos/src/DrawMapSVG.cpp`).
- Three of the four text paths that take the string resolve it by family:
  `MapPainterCairo.cpp:326` (Pango) and `:354` (no Pango, `cairo_toy_font_face_create`),
  `MapPainterSVG.cpp:119` (Pango). The fourth, `MapPainterSVG::ResolveFontFile`
  (`MapPainterSVG.cpp:274`), already accepts a file: an existing path wins, otherwise a fontconfig
  family lookup.
- Two family readers already exist in the repository, one in a demo and one in the tests:
  `TextMetricsAll::ReferenceFontFamily` (`Demos/include/TextMetricsAll.h:220`, FreeType `family_name`)
  and the `Tests/include/TestFontSupport.h` helpers that wrap it. The library has neither.
- The Cairo backend's Pango layout comes from the default font map
  (`pango_cairo_create_layout(draw)`, `MapPainterCairo.cpp:822`). The SVG backend owns a `PangoContext`
  on an FT2 font map, created once per process (`MapPainterSVG.cpp:60-62`).
- Build features are declared per backend: `OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO` is set in
  `cmake/features.cmake:215` and `libosmscout-map-cairo/include/osmscoutmapcairo/meson.build`, and
  declared in `MapCairoFeatures.h.cmake`. The SVG backend already finds FreeType, fontconfig and
  pangoft2 (`libosmscout-map-svg/CMakeLists.txt:16-27`, `libosmscout-map-svg/meson.build:20`); the
  Cairo backend finds none of the three.
- `pango_font_map_add_font_file` is `PANGO_AVAILABLE_IN_1_56`. Ubuntu 24.04 ships Pango 1.52, so the
  mechanism cannot be the only one; the local Pango 1.58 has it.
- `cairo_ft_font_face_create_for_ft_face` is available (cairo 1.18.6 with cairo-ft); nothing in the
  repository uses it today.

## Goals / Non-Goals

**Goals:**

- A configured font file is the face a family-resolving backend draws and measures with, including on
  a host that does not provide that face as an installed family.
- One resolution mechanism, reachable from the Cairo backend and from the SVG backend's Pango path.
- The behaviour does not depend on a process-global mutation where the platform allows a local one.
- The build stays green, in both build systems, on a platform that provides none of the new
  dependencies.

**Non-Goals:**

- Changing what `MapParameter::SetFontName`/`GetFontName` report, or which form a caller should pass.
- Raising the minimum Pango version. Where the local mechanism is unavailable the fallback applies
  (decision 2).
- The FreeType-based font paths (`MapPainterSVG::ResolveFontFile`, AGG, OpenGL) and the font caches of
  the IOS and GDI backends.
- The `font cache keyed by an unquantized double font size` entry of `TODO.md`; the key keeps holding
  the configured name (decision 5).

## Decisions

### 1. The resolution lives in one shared, feature-gated unit of `libosmscout-map`

New `libosmscout-map/include/osmscoutmap/FontNameResolution.h` with its implementation, next to the
`MapParameter` contract it serves. It answers one question: given a configured font name, what should a
family-resolving text path be handed, and which file - if any - has to be made visible to that path.

- **Alternative: one implementation per backend.** Rejected. The repository already has two
  independent family readers and two independent file resolutions; a third and fourth copy would drift
  the way the existing ones did, and `font-management` would have to describe two behaviours.
- **Alternative: normalize inside `MapParameter::SetFontName`.** Rejected. It changes what
  `GetFontName()` reports, which the measurement environments of the backends use as a cache key and
  which the existing spec scenarios read; and a font file is legitimate input for the FreeType-based
  backends, which must keep receiving the path.
- **Alternative: put it in `libosmscout` core.** Rejected: core has no font dependency today and the
  font contract already lives in `libosmscout-map`.
- Risk: `libosmscout-map` is built for every platform including Android and iOS. Mitigated by making
  the unit compile to a no-op that returns the name unchanged when neither FreeType nor fontconfig is
  found, so no platform gains a required dependency.

### 2. The Cairo Pango path gets a per-painter font map, and the file is added to that map

`MapPainterCairo` creates its own `PangoFontMap` and `PangoContext` instead of relying on
`pango_cairo_create_layout(draw)`, adds the configured font file to that map with
`pango_font_map_add_font_file`, and creates its layouts with `pango_layout_new(context)` followed by
`pango_cairo_update_layout(draw, layout)` so the cairo target binding the drawing path relies on is
kept. The measurement path (`MapPainterCairo::GetMeasurementEnvironment`) uses the same context.

When the build has Pango older than 1.56 the same file is added to the process font configuration
instead (`FcConfigAppFontAddFile`, fontconfig), and the backend reports once when that is not possible.

- **Alternative: add the file to the process font configuration on every Pango version.** This is what
  `Tests/include/TestFontSupport.h` does today. Rejected as the primary: it mutates the state of the
  whole process from a library, and on a platform whose default font map is not fontconfig based it
  does nothing unless the process font map is replaced as well (MSYS2 issue 4293 - the tests do exactly
  that, including setting `PANGOCAIRO_BACKEND` before the first font map exists, which a library cannot
  do safely). Risk it leaves: on Pango < 1.56 the effect is process-wide and platform dependent; that
  is the documented fallback, not the contract.
- **Alternative: read the family out of the file and hand over a family name only.**
  `TextMetricsAll::ReferenceFontFamily` does that. Rejected as sufficient: it repairs the name, so the
  right face is found when it is installed, but it cannot serve a file the host does not provide, which
  is the scenario the spec adds. Risk: it is nonetheless part of the chosen approach, because a name
  that is *not* a file still has to resolve by family.
- **Alternative: build the face directly with cairo-ft and bypass Pango.** Rejected for this path:
  Pango performs the shaping and the line layout of this backend
  (`MapPainterCairo.cpp:745`,`:822`,`:901`), so a face alone cannot be handed to it.
- Risk: the layout is currently created on the cairo target (`pango_cairo_create_layout`), so switching
  to `pango_layout_new(context)` + `pango_cairo_update_layout(draw, layout)` touches every layout call
  site of the backend. Mitigation: one helper on the painter creates and binds a layout, and all call
  sites go through it.

### 3. The Cairo variant without Pango loads the file's face directly

That variant has no Pango and therefore no font map to add a file to. It loads the configured file with
FreeType and creates its scaled font from that face
(`cairo_ft_font_face_create_for_ft_face`) instead of asking `cairo_toy_font_face_create` for a family.
A name that is not a file keeps the toy-font-face family path.

- **Alternative: resolve the family and hand it to the toy font face.** Rejected: the toy API resolves
  through the cairo font backend, so a file the host does not provide stays invisible - the same defect
  in a new place.
- **Alternative: add fontconfig registration to this variant as well.** Rejected: it makes a build
  without Pango depend on fontconfig for a weaker guarantee, where the face can be loaded directly.
- Risk: cairo-ft becomes an optional dependency of this variant and its ownership of the FT face and
  the cairo font face has to be defined (the face must outlive the scaled font, and both must be
  released with the painter). Mitigation: the font cache entry owns both, released where the existing
  `cairo_scaled_font_destroy` loop runs (`MapPainterCairo.cpp:293-301`).

### 4. The SVG Pango path is served by the same unit, on its own font map

`MapPainterSVG` already owns a `PangoContext` on an FT2 font map. The configured file is added to that
map through the same helper, and the context stays per painter so that two painters with different
fonts do not share a map.

- **Alternative: leave the SVG path out.** Rejected by the confirmed scope; the defect is identical and
  `font-dependent-test-fonts` can then retire its conversion requirement outright instead of keeping it
  for one backend.
- **Alternative: have the SVG path call the Cairo backend's code.** Rejected: the two backends are
  independent targets; the shared unit of decision 1 is the seam.
- Risk: the SVG backend's Pango variant is not built in every configuration (it needs pangoft2), so the
  change is only exercised where that variant exists. Mitigation: the tasks verify both variants.

### 5. The font cache keeps the *configured* name as its key

`FontKey{fontName, fontSize}` (`MapPainterCairo.h:97`) keeps `fontName` as the caller configured it,
not as it resolved.

- **Alternative: key by the resolved family and file.** Rejected: two different files that carry the
  same family would collide onto one entry, and the entry would then serve the wrong face - the defect
  class this change removes.
- Risk: one entry per spelling of the same face. Accepted; it is what happens today and the cache is
  bounded by the caller's configurations.

### Flow

```
  caller              MapParameter            backend                 shared unit            font stack
    |                      |                      |                         |                      |
    | SetFontName(name) -->|                      |                         |                      |
    |                      |                      |                         |                      |
    | DrawMap() ---------->|--------------------->| GetFont(projection,     |                      |
    |                      |                      |   parameter, size)      |                      |
    |                      |                      |                         |                      |
    |                      | GetFontName() ------>| Resolve(name, ...) ---->|                      |
    |                      |                      |                         | Is name an existing  |
    |                      |                      |                         |   file?              |
    |                      |                      |                         |   no -> return the   |
    |                      |                      |                         |        name, done    |
    |                      |                      |                         |   yes -> read the    |
    |                      |                      |                         |        family out    |
    |                      |                      |                         |        of the file   |
    |                      |                      |<--- family, file -------|                      |
    |                      |                      |                         |                      |
    |                      |                      | add the file to the     |                      |
    |                      |                      | painter's font map ------------------------->|
    |                      |                      | (pango >= 1.56)         |                      |
    |                      |                      | else to the process     |                      |
    |                      |                      | font configuration ------------------------->|
    |                      |                      |                         |                      |
    |                      |                      | cache lookup by the CONFIGURED name + size    |
    |                      |                      |   hit -> return the cached face               |
    |                      |                      |   miss -> build the face from the family      |
    |                      |                      |            (or from the FT face where there   |
    |                      |                      |             is no Pango) and cache it         |
    |                      |                      |                         |                      |
    |<-- labels drawn and measured with that face --|                        |                      |
```

The layout and the measurement of one painter share the context created in the first frame, so what is
measured is what is drawn.

## Risks / Trade-offs

- [The rendered text and the reported metrics of callers that pass a file change, so existing metric
  assertions may move] -> This is the intended behaviour change. Measure before/after with
  `TextMetricsCairoTest`, `TextMetricsSVGTest`, `MapPainterShieldTest`, `TextMetricsAll` and
  `PerformanceTest --driver cairo`; the repository font's face should now be measured on every host,
  so comparisons against the FreeType reference should tighten rather than loosen. Only adjust a
  tolerance that the `text-metrics-api` contract sets, and record the numbers in `verification.md`.
- [The Pango < 1.56 fallback mutates process state and is ineffective on a non-fontconfig font map]
  -> Keep it behind the version check, report once per painter when the file cannot be added, and let
  the fallback path of decision 2's alternatives cover the outcome. A later change may raise the
  minimum Pango; that is an open question below, not a task here.
- [Adding FreeType, fontconfig and pangoft2 to the Cairo backend breaks a platform whose packages are
  named differently] -> Follow the SVG backend's pattern: optional dependency, `target_exists` in CMake
  (`cmake/features.cmake:211`), `required: false` in Meson (`meson.build:182`,`:214`), feature macros in
  `MapCairoFeatures.h.cmake`, and a build that compiles to the current behaviour when a dependency is
  absent.
- [Two build systems have to register the same new sources and the same new features] -> The shared
  unit is registered in `libosmscout-map/CMakeLists.txt` and `libosmscout-map/meson.build`, the Cairo
  features in `cmake/features.cmake`, `MapCairoFeatures.h.cmake` and
  `libosmscout-map-cairo/include/osmscoutmapcairo/meson.build`. `TODO.md` records that nothing checks
  the two rosters match; the tasks verify both builds instead.
- [The tests' helpers are the only exercised path today, so removing them removes a working
  workaround] -> Remove them only after a new test asserts the library's behaviour on a host that does
  not provide the file as an installed family; that test is what replaces the helper.
- [The IOS and GDI backends have their own font caches and the same class of name handling] -> Out of
  scope here and recorded in `TODO.md`; the shared unit is written so those backends can adopt it
  later without a second mechanism.

## Migration Plan

No API migration: `MapParameter::SetFontName` keeps both forms and the callers do not have to change.
The behaviour change is the migration, and it is observable only in the rendered labels and the
reported metrics of callers that pass a file.

Rollback: the new dependencies are optional and feature-gated, so reverting the two `GetFont` bodies
(`MapPainterCairo.cpp:308-376`) and the SVG Pango resolution restores today's behaviour without
touching the build descriptions.

## Open Questions

- Whether a later change raises the minimum Pango version to 1.56 and drops the process-wide fallback
  of decision 2. Deferrable: the fallback is correct, only less local.
- Whether the font file should also be reported through a diagnostic the caller can query, rather than
  only logged. Deferrable: no spec scenario depends on it; `font-management` asks for a report only.
