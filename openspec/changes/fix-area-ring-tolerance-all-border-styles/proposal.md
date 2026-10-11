# Proposal

## Why

A painter decides per ring whether the ring can be visible by extending the ring's bounding box by a
tolerance derived from one of the border styles the ring resolves, while the drawing step draws every
border style the ring resolves. A ring whose first border style is drawn at an offset is therefore
decided with no tolerance at all, so an area whose border crosses the viewport edge can be dropped
from the frame although its border would have been drawn (TODO §99).

The same tolerance rule is mirrored by the OpenGL backend's own area step, so the deviation exists
once in the shared area preparation and once in the OpenGL backend, and the two can drift apart
independently. No shipped stylesheet declares an offset area border today, which is why the defect is
latent rather than visible; any stylesheet that does declare one loses the borderline geometry of
such an area.

## What Changes

- The tolerance a ring is decided with SHALL cover every border style the ring resolves, including a
  style that is drawn at an offset, so a ring whose drawn border reaches the view is not rejected.
- The early rejection of areas SHALL remain conservative with respect to the per-ring decision: the
  bound the early decision extends an area by SHALL be at least the tolerance any per-ring decision
  can use for the loaded stylesheet and the current zoom level.
- The OpenGL backend's area step SHALL decide a ring with the same tolerance rule as the shared area
  preparation, so the backend's own decision does not keep a narrower rule than the painter it
  stands in for.
- For a stylesheet whose border styles are already covered by today's tolerance, the prepared area
  entries, the draw order and the rendered output SHALL be unchanged; the change only widens the
  decision where it was narrower than the drawing step.
- Out of scope, and recorded for their own changes: the OpenGL area step's skip of rings that have a
  border but no fill style (TODO §98), the vendored triangulator's remaining `assert`-only
  invariants (TODO §100) and the OpenGL node-path triangulation crash (TODO §97, implemented on an
  open branch).

## Capabilities

### New Capabilities

None. The behaviour of the per-ring visibility decision belongs to the existing area-culling
contract.

### Modified Capabilities

- `map-painter-area-culling`: the tolerance requirement is extended from the early rejection to the
  per-ring visibility decision - the tolerance a ring is decided with SHALL cover every border style
  the ring resolves, not a single one of them, and the early decision's bound SHALL stay at least as
  large as that tolerance.

## Impact

Affected code:

- `libosmscout-map/src/osmscoutmap/MapPainter.cpp` — the per-ring visibility decision of area
  preparation, its tolerance and the invariant that relates it to the early rejection of areas.
- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` — the OpenGL area step's
  per-ring decision, which today mirrors the shared painter's expression and would keep the narrow
  rule on its own.
- `libosmscout-map-opengl/include/osmscoutmapopengl/AreaVisibility.h` and
  `libosmscout-map-opengl/src/osmscoutmapopengl/AreaVisibility.cpp` — the backend's visibility
  predicate, if the tolerance it receives has to be derived from a set of border styles rather than
  a single width.
- `libosmscout-map/include/osmscoutmap/MapPainter.h` — only if the widened tolerance has to be
  shared between the painter and its backends rather than recomputed in each.

Tests and build:

- `Tests/src/OpenGLAreaVisibilityTest.cpp` (extend) and, if the shared painter's decision is best
  pinned headlessly, a new case in `Tests/src/` — a ring whose first border style is drawn at an
  offset is kept when its drawn border reaches the view, and one whose drawn border does not reach it
  still contributes nothing.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register any new target in both build systems.

Bookkeeping:

- `TODO.md` — the entry this change closes (TODO §99), and the file/line references that go stale.

Not affected: the stylesheets, the other backends' own preparation, the fill style decision and the
skip behaviour of the OpenGL area step.

Related work in flight: `fix-opengl-area-visibility-cull` added the OpenGL tolerance requirement to
the same capability and still owes its rendered comparison (its task 4.2), which the OpenGL node-path
crash (TODO §97, open branch `harden-polygon-triangulation`) currently blocks. This change touches
the same capability's tolerance rule, so the two deltas have to merge in the order they land.
