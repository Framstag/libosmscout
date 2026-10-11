# Proposal

## Why

A loaded style configuration costs memory and build time in proportion to the number of types the type
configuration defines, not to the styles a stylesheet actually uses. The shipped type set is held at its
released size by parking 856 further definitions, and unparking them is gated on this cost being
understood and bounded; without that, every type added later makes loading and switching a stylesheet
more expensive for every renderer.

## What Changes

- The resources a loaded style configuration consumes SHALL be governed by the styles the loaded
  stylesheet references, not by the number of defined types — both the time to build the configuration and
  the memory it retains.
- Resolving the style of an object type at a magnification level SHALL return exactly the styles it
  returns today; the observable rendering output is unchanged.
- Loading the style configuration SHALL stay correct when a stylesheet references a type that the type
  configuration does not define, as it does today.
- The public accessor contract through which a painter resolves styles SHALL remain unchanged, so no
  backend has to be adapted and no caller has to be migrated.
- A measurement of the style-configuration build cost SHALL exist that reports the figure for the shipped
  type configuration, so that the type-set growth of the parked definitions can be judged before they are
  enabled.
- No database, type-definition or file format changes; no re-import is required.

## Capabilities

### New Capabilities

- `style-configuration`: what a stylesheet load produces and what resolving a style for an object type at
  a magnification level guarantees — the styles returned, the tolerance for references to undefined types,
  and the property that the cost of a load and the memory of a loaded configuration follow the styles a
  stylesheet references rather than the number of defined types.

### Modified Capabilities

- None. The requirements above are new; the rendered output and every existing style behavior stay as
  they are.

## Impact

**Modules and files expected to change**

- `libosmscout-map/include/osmscoutmap/StyleConfig.h` — the post-process lookup tables and the style
  resolution accessors of a loaded configuration.
- `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` — the build and post-process steps that populate them,
  including the per-style-class step that currently walks the type × magnification space.
- `libosmscout-map/src/osmscoutmap/MapPainter.cpp`, `libosmscout-map/src/osmscoutmap/MapPainterStatistics.cpp`,
  `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` — the callers of the affected
  accessors; they stay source-compatible (see the accessor-contract bullet above).
- `Demos/src/SymbolsAll.cpp` — the existing `--list` path is the measurement entry point for the
  stylesheet-plus-type-definition load cost.
- `Tests/src/StyleConfigLookupCostTest.cpp` (new; the existing style tests are one file per concern —
  `StyleConfigSymbolsTest.cpp`, `StyleConfigVisibilityBoundsTest.cpp`, `StyleLoadResilienceTest.cpp`) — the
  case pinning the resolution result and the cost property; registered in both `Tests/CMakeLists.txt` and
  `Tests/meson.build`.
- `stylesheets/**` — read only; no stylesheet edit is part of this change.

**Systems and contracts**

- Public API: `StyleConfig` accessors keep their signatures and their results; the internal tables are not
  part of any public contract.
- Database, type definition and file format: untouched; no version bump under
  `guidelines/FileFormatVersion.md`.
- Renderers: all backends that resolve a style through `StyleConfig` (Cairo, Qt, AGG, SVG, Skia, OpenGL,
  GDI, DirectX, iOS, binding) benefit without an edit.

**Sequencing constraint (from the live surface at the time of this proposal)**

- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` is being rewritten by PR #1784
  (`fix-label-flicker`), so the accessor contract staying fixed is also what keeps this change off that
  file. `MapService.cpp` and `DataTileCache.h` are live in PR #1860, #1861 and #1862 and are outside this
  change's surface.
- The type-count magnitudes in `TODO.md` §22 describe the type set with the parked definitions active;
  with the shipped set the same tables are smaller but the dependence on the type count is the same, which
  is what this change removes.
