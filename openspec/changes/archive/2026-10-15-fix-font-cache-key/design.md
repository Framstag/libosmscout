# Design

## Context

See `proposal.md` - Why. The two affected caches today:

- Cairo: `MapPainterCairo.h:89` `using FontMap = std::unordered_map<double,CairoFont>;`, filled and
  read by both `GetFont` variants in `MapPainterCairo.cpp` (`:300` Pango, `:323` the non-Pango
  `cairo_scaled_font_t` path). The key is
  `fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize())`; the family comes from
  `parameter.GetFontName()` (`:319`, `:342`). Entries are freed in the destructor and `Close()`
  (`:288-299`, `:689`).
- SVG: `MapPainterSVG.h:60` `using FontMap = std::unordered_map<size_t,PangoFontDescription*>;`, filled
  and read by `MapPainterSVG.cpp:99-118`; family from `parameter.GetFontName()` (`:112`), freed in the
  destructor (`:74`).

Both painters are long-lived (`MapRenderer.h:122` holds one `MapPainterQt`; the Cairo painter is used
the same way), so an entry resolved for an earlier font name survives for the whole session.

The label-reuse work already derives the reuse key of a *measurement* from the font name
(`MapPainterCairo::GetMeasurementEnvironment`, `MapPainterCairo.cpp:1381-1400`, built from
`parameter.GetFontName()`). The painter therefore re-measures with the new font while still drawing the
old one - the inconsistency the spec delta of `map-painter-label-reuse` closes.

The two reference implementations already solve this: Qt keys on
`FontDescriptor{fontName,fontSize,weight,italic}` (`MapPainterQt.h:63-73`), DirectX on
`GetFontHash(fontName,fontSize)` (`MapPainterDirectX.cpp:387`).

Tests: `Tests/src/TextMetricsCairoTest.cpp` and `Tests/src/TextMetricsSVGTest.cpp` set up one
`MapParameter` per measure call and create a fresh painter per `TEST_CASE`, so neither exercises a
font-name change on a live painter. The repository ships exactly one font file
(`libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf`), and
`font-dependent-test-fonts` requires font-dependent tests to measure the font the repository ships
rather than one the host provides.

## Flow

A font-name change on one long-lived painter, before and after the fix:

```
  frame 1                                  frame 2 (font name changed A -> B)
  -------                                  ------------------------------------
  app        painter                       app        painter
   | set name A |                           | set name B |
   |----------->|                           |----------->|
   | draw       |                           | draw       |
   |----------->| GetFont(A, size)          |----------->| GetFont(B, size)
   |            | key = size                |            | key = size (bug) / (B, size) (fix)
   |            | miss -> construct F(A)    |            | bug: HIT  -> returns F(A)  <-- wrong font
   |            |                           |            | fix: MISS -> constructs F(B) <-- right font
   |            |                           |            |
   |            | label measurement key     |            | label measurement key
   |            | includes name A           |            | includes name B
   |            |                           |            |
   |            | draws F(A), measures A    |            | bug: draws F(A), measures B  <-- mismatch
   |            |                           |            | fix: draws F(B), measures B
```

The measurement half is already correct (it is keyed by `GetMeasurementEnvironment`, which includes
`parameter.GetFontName()`); only the resolved font is not, which is why the two disagree under the bug.

## Goals / Non-Goals

**Goals:**

- A painter that is handed a different font name draws with the font of the new name.
- The font the painter resolved for a name and the label measurement it reports for that name agree.
- Rendered output for an unchanged font name is unchanged.
- The fix is verifiable without depending on which fonts the machine provides.

**Non-Goals:**

- Quantizing or bounding the font cache (the separate TODO entry "Font caches are keyed by an
  unquantized `double` font size and are never bounded"). The size component of the key keeps its
  current value, only the name component is added.
- The same defect in the IOS backend (`MapPainterIOS.h:170` `std::map<size_t,Font*>`), which needs an
  Apple run to verify; recorded as a TODO entry instead.
- Resolving a font *file* path to a family (`MapPainterCairo.cpp:319` family handling), a separate
  TODO entry.
- Changing the label measurement cache or its environment.

## Decisions

### D1: How a cache entry is selected

**Chosen: a composite key per backend** - Cairo gains a private
`struct FontKey { std::string fontName; double fontSize; }` with equality and a hash (or an ordered
comparison, mirroring Qt's `std::tie`), SVG the equivalent with its `size_t` size; `FontMap` becomes
`std::unordered_map<FontKey,...>`.

Alternatives:

1. *Nested map* (`unordered_map<std::string, unordered_map<double,...>>`) - same behaviour, one extra
   indirection per lookup and two allocations per new name; rejected as no benefit over the flat key.
2. *Clear the cache when the font name changes* (track the last name, free all entries) - the smallest
   diff, but it frees Pango/cairo font objects in the middle of a session (today only the destructor
   and `Close()` do), re-resolves every size after any name change, and loses the entries of the other
   name; rejected.
3. *Drop the cache* - removes a real optimization for an unchanged name; rejected.
4. *Reuse the Qt `FontDescriptor` shape* - the two backends hold different value types (Cairo's value
   is conditional: `PangoFontDescription*` or `cairo_scaled_font_t*`), and publishing a shared type for
   a private cache widens the API; a per-backend private key is enough and matches how Qt keeps its
   descriptor private.

The chosen key keeps the flat `unordered_map` and the existing destruction loop unchanged (it iterates
the map, so it frees every entry regardless of key type).

### D2: What "the same font" means for the key

Only the name and the scaled size are keyed, matching the inputs `GetFont` actually resolves from.
Weight, slant and font options are not painter parameters today (Cairo hardcodes
`CAIRO_FONT_SLANT_NORMAL`/`CAIRO_FONT_WEIGHT_NORMAL`, `MapPainterCairo.cpp:342-343`), so keying them
would be dead state. If a style ever carries them, they join the key - noted as a follow-up rather than
added now.

### D3: How the behaviour is verified without a second bundled font

**Chosen: a small diagnostic counter on each painter plus a host-dependent measurement comparison.**

- The painter counts the fonts it has constructed for a request (`GetResolvedFontCount()`, documented
  as a diagnostic for tests). The test drives one painter with name A, again with A at the same size
  (count unchanged - the entry is reused), then with B (count +1). A fix that clears on a name change
  also satisfies this: the count tracks constructions, not cache size. This is deterministic: it needs
  no second font to exist, because a font is constructed for a name whether or not the name resolves.
  The SVG case needs two resolvable families for its two names (the pango-less variant resolves a
  family through fontconfig and caches the face per file), so it runs only where fontconfig is
  available and reports an `INFO` line and passes otherwise; the Cairo case constructs a font for any
  name and runs everywhere.
- A second case measures the same label through one painter under two names and compares each result
  with a fresh painter for that name - the exact user-visible contract. The drawn glyphs are resolved
  by the same call the measurement uses (`MapPainterCairo::Layout`, `MapPainterSVG::Layout`), so the
  ink metrics cover the drawn font; a full render is not used because no public API puts a label into
  `DrawMap` without map data. The comparison only discriminates when the two names resolve to
  different fonts, so the case first asks fontconfig whether they do and reports an `INFO` line and
  passes when they do not, mirroring the existing family-resolution check of
  `TextMetricsCairoTest.cpp:139-150`.

Alternatives:

1. *Only the measurement comparison* - directly observable, but on a host whose fallback font equals
   the bundled family the case passes with and without the fix; rejected as the sole gate (kept as the
   second case).
2. *Ship a second font file and register it* - fully observable and metric-based, but adds a binary
   asset to the repository for one test; rejected.
3. *Compare against the painter's own measurement environment* - the environment already includes the
   name (`MapPainterCairo.cpp:1389`), so it never differs; rejected, it tests nothing.
4. *Make the test a friend of the painter and read the cache* - asserts cache size, which the
   clear-on-change alternative would fail; rejected as implementation-prescribing.

The diagnostic method is a plain const accessor on both painters with a comment stating that it exists
for tests and carries no rendering behaviour.

## Risks / Trade-offs

- **The cache can now hold one entry per (name, size) pair** - a font-name change is rare and both
  backends already free everything on `Close()`/destruction; the unbounded-growth TODO entry covers the
  general case. Mitigation: nothing further in this change.
- **A new public accessor on two exported painters** - keep it documented as a diagnostic, no behaviour,
  and the only caller is the test.
- **Cairo's non-Pango variant must be covered too** - both variants write the same `fonts` map, so one
  key type covers both; the tasks verify the non-Pango build compiles and behaves (`build` with and
  without `OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO`).
- **SVG's cached value is a `PangoFontDescription*` freed in the destructor** - the key change must not
  break that loop; it iterates the map and stays valid.
- **IOS keeps the defect** - documented as a TODO entry so it is not lost.
- **A host whose fallback font equals the bundled family** - the measurement comparison degrades to an
  `INFO` pass, but the deterministic counter case still catches the bug.

## Migration Plan

No data, format or public-behaviour change; no database needs re-importing. The change is a rendering
correctness fix, so the rollback is a revert.

## Open Questions

- Whether the font *weight* and *slant* the Cairo backend hardcodes should become painter parameters at
  some point; that would extend the key. Deferrable - it does not change this change's specs, approach
  or tasks.
