## Context

See proposal.md — Why for motivation. Current state:

- `MapPainterSVG` (`libosmscout-map-svg/src/osmscoutmapsvg/MapPainterSVG.cpp`) has two measurement paths guarded by `OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO`. The Pango path (default) measures real metrics via `pango_layout_get_pixel_extents` and per-run glyph advances, and was fixed by `fix-text-metrics-shields`. The `#else` (non-Pango) path approximates: `Layout()` computes `width = length(character) * height * AverageCharacterWidth` and `ToGlyphs()` advances each glyph by `height * AverageCharacterWidth` — a constant per-character advance regardless of glyph. Measured: label 340 px vs 216 px ink at 30 px font (~57% over).
- Pango is optional in CMake (`libosmscout-map-svg/CMakeLists.txt`: `if(PANGOFT2_FOUND)`); the non-Pango path exists for builds without Pango. The library currently links only `OSMScout::OSMScout` and `OSMScout::Map` (+ optionally Pango/Harfbuzz) — no FreeType. Meson builds the same two-path structure.
- `TextMetricsAll` (`Demos/src/TextMetricsAll.cpp`) renders a reference label with FreeType and reports `height` from `face->size->metrics.height` (the font box: 35 px at 30 px font), while backends report ink height (23 px). The `text-metrics-api` spec, the `TextMetricsCairoTest`, and the shield tests all use ink semantics; the reference disagrees with what it is meant to verify.
- The Cairo backend already solves the non-Pango problem: its `#else` path measures with FreeType glyph metrics directly (per TODO evidence from `fix-text-metrics-shields`).

## Goals / Non-Goals

**Goals:**
- Non-Pango SVG build measures with real font metrics; widths/heights no longer driven by character count.
- `TextMetricsAll` reference reports ink height, agreeing with backends.
- Test coverage for both, so drift fails CI.
- Pango SVG path byte-for-byte unchanged behavior.

**Non-Goals:**
- Unifying every backend on one measurement engine (existing per-backend paths stay).
- Changing the `text-metrics-api` contract (ink semantics stays the contract).
- Full wrapping/layout parity between Pango and non-Pango SVG (only metric consistency is in scope).

## Decisions

### D1: Non-Pango SVG measurement uses FreeType glyph metrics, mirroring Cairo

The non-Pango `Layout()`/`ToGlyphs()`/`GlyphBoundingBox()` measure per-glyph with FreeType: advance width from `FT_GlyphSlot` metrics, ink box from glyph extents, height from ink bbox. FreeType becomes a dependency of `libosmscout-map-svg` in both build systems (same pattern as `libosmscout-map-cairo`). Flow:

```
MeasureText(text, fontSize)
  -> Layout(): for each character
       FT_Load_Char -> glyph metrics (advance, ink bbox)
       accumulate width, ink height
  -> ToGlyphs(): glyph position = accumulated advances
  -> GlyphBoundingBox(): ink box of the glyph
```

- **Alternative A (chosen)**: FreeType per-glyph measurement in the non-Pango path. Keeps Pango optional (existing configure contract), reuses the proven Cairo pattern, no feature removal, satisfies the `text-metrics-api` requirement that all backends implement measurement consistently.
- **Alternative B**: Make Pango mandatory for the SVG backend and delete the non-Pango variant. Simplest code, but removes a supported configuration (Pango is currently optional in CMake/meson) and would break builds on platforms without Pango; also diverges from Cairo, which keeps its non-Pango FT path.
- **Alternative C**: Keep the approximation but clamp/label it as approximate. Rejected — the measured 57% width error violates the metric consistency requirement; cosmetic fixes cannot close it.

Risk assessment: A is low risk — FT is already a project-wide dependency (Cairo, AGG, Qt paths use it), and the change is confined to the `#else` branch. Main risk is dependency wiring drift between CMake and meson (same risk class as the existing Pango wiring); mitigated by keeping both build files structurally identical and by a CI config without Pango exercising the branch.

### D2: TextMetricsAll reference reports ink height

The FreeType reference computes label height from the ink bounding box of the rendered glyphs (union of glyph ink extents), matching how backends report ink height.

- **Alternative A (chosen)**: Report ink height (bbox of rendered glyphs) as the reference height. Conforms to the existing ink contract without touching backends; the demo then validates what the tests already assert.
- **Alternative B**: Change the contract to box height and update all backends. Large blast radius (every backend's height semantics, existing tests, shield code) for no user-visible gain — rejected.
- **Alternative C**: Report both box and ink height in the demo. Adds tooling surface and leaves the contract ambiguous; the demo's job is to be a single authoritative reference — rejected.

Risk assessment: A is low risk — a local change in the demo's FreeType reference code; margins in `TextMetricsCairoTest` already tolerate small per-glyph differences, and ink-vs-ink comparison is tighter than the current box-vs-ink comparison.

## Risks / Trade-offs

- [FT dependency added to SVG backend] → Already used by every other measurement path; both build systems must link it identically (CMake `freetype` target / meson `ftDep`).
- [CI builds without Pango are rare] → The SVG non-Pango branch may go unexercised; mitigation: a unit test measuring with the non-Pango path is required, and the change notes that a no-Pango config is the verification target.
- [Pango shaping differences remain (e.g. digit alternates)] → Already tolerated by existing test margins; this change does not reopen that decision.
- [Per-glyph FT vs Pango advance rounding] → Same class of small difference the `TextMetricsCairoTest` margins already absorb; margins stay unchanged.

## Migration Plan

No data migration. Behavior change in two components; rollback is a revert of the SVG measurement code and the demo reference change. No format or file-version impact.

## Open Questions

- Should the SVG non-Pango path also gain contour-label handling parity with Pango? Out of scope here — metric consistency only; revisit if a no-Pango layout bug surfaces.
