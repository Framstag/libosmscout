# Proposal

## Why

The Java client's render built its `MapParameter` and stated no label line-wrapping value at all. The
library defaults are a minimum character count of 5, a maximum of 15 and fit-to-area enabled — but a
fit-to-width of 8000 px, which does not bind a label in a viewport of any phone or desktop size. The pixel
bound is the one that makes wrapping happen for a label longer than the character bound, so in this app a
long street or place name was drawn on one line and ran across the map, however narrow the viewport was.
The Qt plane renderer states all four values; the Java client stated none.

## What Changes

- The render SHALL state the label line-wrapping bounds rather than inherit them, with fit-to-width the
  shorter side of the render buffer: minimum character count 5, maximum character count 15, fit-to-area
  enabled, fit-to-width the shorter buffer side.
- The bound SHALL be the shorter side of the buffer the caller asked for, because that buffer carries the
  pan overrun and is larger than the visible viewport.
- The decision SHALL live in one dependency-free place that both the bridge uses and a host test can
  exercise, so the stated values cannot drift from the ones the render applies, and so the difference from
  the non-binding default (8000 px) is pinned without a database.
- A label SHALL still only be broken at a word boundary, so a one-word label SHALL stay on one line
  whatever its width — the rule the label layouter already follows.

## Capabilities

### New Capabilities

<!-- None. -->

### Modified Capabilities

- `map-rendering`: one added requirement, the label line-wrapping bounds the JNI render states. The
  existing render requirements (its signature, its DPI, its return value, its overlays) are unchanged.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/render_label_bounds.h` — new dependency-free header holding the four values
  as one decision.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the shared render body (`renderMapIntoPixels`) applies
  the decision to its `MapParameter`.
- `Tests/src/RenderLabelBoundsTest.cpp` — new host test for the decision and its difference from the
  library default fit-to-width.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the test in both build systems.

No Java API change, no native signature change, no new native method, so no JNI declaration and no
`javac`/jar change. No database, style or file format change, so no `FileFormatVersion.md` version bump
applies. No new dependency.

Composes with `map-cairo-label-wrapping` (which makes the wrapping reach the Cairo text layout without a
text stack) but does not depend on it: with a text stack present the wrapping already works, and this
change is what asks for it.

## Note on the reference

The reference commit (`e303556e2`) calls the four values "the library defaults, whose 8000 px fit-to-width
never binds" and then sets all four explicitly. Three of the four coincide with the defaults; only
fit-to-width changes. The explicit setting of all four is kept, because the render should not depend on a
default it means to state — the wording above states that truthfully.
