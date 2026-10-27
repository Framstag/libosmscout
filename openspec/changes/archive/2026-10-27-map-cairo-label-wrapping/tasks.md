# Tasks

## 1. The wrapping layout without a text stack (spec: cairo-label-wrapping)

- [x] 1.1 In `libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp`, replace the single
  `cairo_scaled_font_text_extents()` measurement of the layout without Pango with a layout that measures an
  average character width from the resolved font, asks `MapPainter::GetProposedLabelWidth` for the line
  width the parameters allow, and breaks the text at word boundaries. Verify: a build with
  `OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO` undefined lays out a multi-word label into lines.
- [x] 1.2 Keep a word wider than the line on a line of its own and a label below the minimum character count
  on one line. Verify: both rules are observable in the host cases of task 3.
- [x] 1.3 Extend the native label struct in
  `libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h` with the lines, each line's ink
  extents and the baseline advance. Verify: the struct's fields are documented, and the existing fields keep
  their meaning.
- [x] 1.4 Report the widest line's ink width and the stacked height from the layout, and the unwrapped ink
  height and draw position for a label that does not wrap. Verify: the wrapped label's height grows and the
  unwrapped label's values are unchanged.

## 2. Drawing and glyph reporting per line (spec: cairo-label-wrapping)

- [x] 2.1 Draw one line per entry, centred in the label rectangle, in `DrawLabel` for both the text style
  and the shield style, and in `ToGlyphs`. Verify: a wrapped label is drawn on more than one baseline and a
  one-line label is drawn where it was.
- [x] 2.2 Verify the wrapped label's rect and the drawn lines agree, so a caller that places the label by
  its reported size places what is drawn.

## 3. Public measurement overload (spec: text-metrics-api)

- [x] 3.1 Declare `MeasureText(projection, parameter, text, fontSize, objectWidth, enableWrapping)` on
  `MapPainterCairo` and implement it, passing the object width and the wrapping flag to the layout. Verify:
  the existing measurement's result is unchanged for the same arguments.
- [x] 3.2 Document the overload where it is declared: the object width (0 for a point label) and the
  wrapping flag, and that the call makes the renderer's layout observable.

## 4. Host tests (spec: cairo-label-wrapping, spec: text-metrics-api)

- [x] 4.1 Append to `Tests/src/TextMetricsCairoTest.cpp`, inside the `!defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)`
  guard, a parameter helper whose character bounds do not bind and a helper that counts the distinct
  baselines of a measurement.
- [x] 4.2 Add the case of a multi-word label wrapped into a width of half the unwrapped width: more than one
  baseline, narrower than the unwrapped label, taller than it, and one glyph fewer per line break.
- [x] 4.3 Add the case of an over-wide single word and of a label below the minimum character count: both
  stay on one line with the unwrapped width and height.
- [x] 4.4 Build and run `TextMetricsCairoTest` in a configuration without Pango and verify every case
  passes.

## 5. Build and regression verification

- [x] 5.1 Build the CMake build with the Cairo backend and the tests; verify it compiles without errors and
  without new warnings from the touched files.
- [x] 5.2 Build the Meson build the same way; verify it compiles without errors.
- [x] 5.3 Run `TextMetricsCairoTest` and the label/text-metrics tests of the suite and verify no existing
  case regresses (including the cross-backend measurement cases, which state no wrapping).
- [x] 5.4 Run `openspec validate "map-cairo-label-wrapping" --strict` and verify the change validates.

## 6. Change hygiene

- [x] 6.1 Verify the diff touches only the files in the proposal's Impact section.
- [x] 6.2 Verify the Pango variant's behaviour is untouched: its wrapping and its measurement are what they
  were.
