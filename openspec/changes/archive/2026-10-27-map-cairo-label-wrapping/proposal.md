# Proposal

## Why

The Cairo backend has two text stacks: one built on Pango, and one that draws glyphs through cairo
directly. The Pango variant hands the wrapping parameters to `pango_layout_set_wrap(PANGO_WRAP_WORD)`, so a
long multi-word label is broken into lines. The variant without a text stack measured the whole text with a
single `cairo_scaled_font_text_extents()` call and read neither the object width nor the wrapping flag, so
every label of that variant was drawn on one line: the map's minimum and maximum character count, its
fit-to-area and its fit-to-width never reached the text layout at all.

A build configured without Pango is not exotic — it is the configuration of the font-dependent tests and of
every host that does not provide Pango — so a label that the other stacks wrap runs across the map there.

## What Changes

- The Cairo text layout without a text stack SHALL honor the line-wrapping map parameters: it SHALL measure
  an average character width from the resolved font, ask the base class for the line width the parameters
  allow, and break the text into lines at word boundaries.
- A word SHALL be the smallest unit of a break: a word wider than the allowed line width SHALL stay on a
  line of its own, and the minimum character count SHALL keep a short label on one line — the rule
  `PANGO_WRAP_WORD` follows in the other variant.
- The label SHALL carry its lines, each line's ink extents and the baseline advance, so drawing and glyph
  reporting work per line, and the label SHALL report the widest line's ink width and the stacked height
  so the label layouter places and culls the size that is drawn.
- A label that does not wrap SHALL keep the ink height and the draw position it had.
- The public measurement SHALL gain an overload that states the object width and the wrapping flag, so a
  host can measure the layout the renderer would draw.

## Capabilities

### New Capabilities

- `cairo-label-wrapping`: the label layout of the Cairo backend without a text stack, its word-boundary
  wrapping, the per-line drawing and the measurement it reports.

### Modified Capabilities

- `text-metrics-api`: one added requirement, the measurement overload that states the object width and the
  wrapping flag. The existing measurement (which states neither) is unchanged, so its cross-stack equality
  requirement is untouched.

## Impact

Affected files and modules:

- `libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h` — the native label gains its lines,
  their ink extents and the baseline advance; the wrapping measurement overload is declared.
- `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp` — the layout without a text stack wraps,
  `DrawLabel` and `ToGlyphs` draw one line per entry, and the measurement overload is implemented.
- `Tests/src/TextMetricsCairoTest.cpp` — host cases for the wrapping, run in the build that has no Pango.

No database, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency; the wrapping uses the font and the base class only. No existing signature changes — the new
measurement is an overload.
