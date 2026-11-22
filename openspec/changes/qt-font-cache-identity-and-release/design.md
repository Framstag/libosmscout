# Design

## Context

See proposal.md — Why. The current state that shapes the approach:

- `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp:61-84` (`MapPainterQt::GetFont`) builds the
  cache identity from the requested font name plus
  `fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize())`. The product is a `double`, the
  identity field `FontDescriptor::fontSize` is a `size_t`
  (`libosmscout-map-qt/include/osmscoutmapqt/MapPainterQt.h:62-73`), so the identity is the truncation
  of the product — the pixel grid, reached by accident of the field's type.
- `MapPainterQt.cpp:55-59` (destructor, `// TODO: Clean up fonts`) and
  `MapPainterQt.cpp:886-891` (`StyleSheetChanged`, which clears `patternImages` and `patterns`) are the
  two places the painter drops frame state; the cache `MapPainterQt.h:85` is dropped in neither. The
  painter is a long-lived member of the Qt client
  (`libosmscout-client-qt/include/osmscoutclientqt/MapRenderer.h:122`).
- `GetFont` is called per label: `GetFontHeight` (`MapPainterQt.cpp:264`) and the layout path
  (`MapPainterQt.cpp:423`, reached by the public `MeasureText` and by `DrawMap`).
- The neighbouring label measurement cache is bounded, with an environment key that includes the font
  name and the device DPI (`MapPainterQt.cpp:1096-1107`); `openspec/specs/map-painter-label-reuse/spec.md`
  states its bound, its least-recently-used eviction and its switch-off, with
  `LabelLayouter::defaultMeasurementCount=4096`
  (`libosmscout-map/include/osmscoutmap/LabelLayouter.h:339`). It is bounded because its key carries the
  label text; a font identity carries no such unbounded dimension.
- What the other backends do today (all without a bound): Cairo releases its fonts in
  `StyleSheetChanged` and the destructor (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp:937-971`)
  and keys them by a `double` size (`MapPainterCairo.h:113-134`); SVG frees them only in the destructor
  (`MapPainterSVG.cpp:138-160`); IOS releases its images but not its `Font*` entries
  (`MapPainterIOS.mm:47-56`); DirectX releases its text formats in the destructor and on device loss
  (`MapPainterDirectX.cpp:1063-1078`); GDI's map lives in the per-buffer render object
  (`MapPainterGDI.cpp:152`,`:166-179`); AGG creates and deletes its font engine inside `DrawMap`
  (`MapPainterAgg.cpp:729`,`:754-755`,`:772-773`).
- The precedent for an assertable diagnostic is `MapPainterCairo::GetResolvedFontCount()`
  (`libosmscout-map-cairo/include/osmscoutmapcairo/MapPainterCairo.h:335`), used by
  `Tests/src/TextMetricsCairoTest.cpp:414-432` to distinguish a served font from a resolved one. The Qt
  painter has no counterpart; `Tests/src/TextMetricsQtTest.cpp` holds one case today.
- Qt painter tests need a display: `QT_QPA_PLATFORM=offscreen` (AGENTS.md).

## Goals / Non-Goals

**Goals:**

- State the identity rule the Qt painter selects a cached font by, and keep it at the pixel grid, so the
  number of fonts a session resolves follows what is drawn and not the number of labels.
- Release the retained fonts where the painter already drops frame state and at destruction.
- Make both assertable: observable counters plus Catch2 cases in the existing Qt test target.

**Non-Goals (design-level):**

- A cache bound, for the reason the proposal gives.
- The other backends' identities and release points (see proposal.md — out of scope).
- Font-name resolution (`fix-font-cache-key`, merged) and the label measurement cache itself.

## Decisions

### 1. The identity of a cached font is an explicit rounding to the pixel grid

**Chosen (A):** the identity keeps the resolution it has today — the scaled size truncated to whole
device pixels — as a named operation of the painter, documented where the identity type is declared.
`FontDescriptor::fontSize` stays an integral pixel size, so the Qt backend's rendered output is
unchanged by this change.

Alternatives:

- **(B)** Keep the exact scaled size as the identity and let the release do the work. Rejected: the
  layout path derives an automatic size from each object's own projected box
  (`libosmscout-map/src/osmscoutmap/MapPainter.cpp:445-466`), so a frame alone produces a continuum of
  identities; the painter would resolve a font per label, which is the cost the cache exists to avoid.
  This is what the Cairo and SVG painters do today (`MapPainterCairo.h:113-134`).
- **(C)** Quantize coarser than a pixel, for example to whole millimetre steps or a relative step.
  Rejected: a difference of one pixel is visible, so a coarser step changes which size a label is drawn
  at; it would also break the equality of measurement and drawing asserted by the third requirement
  unless the measurement key quantized identically.

Risk of (A): the identity resolution must not exceed the resolution of the label measurement, or a
painter could draw a font whose metrics differ from the measurement it reuses. Mitigation: the identity
is integral pixels, the measurement path uses the same resolved font for measurement and drawing, and
the third requirement's second scenario asserts the two against the font the counter reports.

### 2. The painter releases its resolved fonts; it does not bound them

**Chosen (A):** the painter gets an explicit release — a public method that drops every resolved font it
retains — and performs that same release in `MapPainterQt::StyleSheetChanged`, where the pattern images
of a replaced stylesheet are already dropped, and in the destructor, replacing the stale
`// TODO: Clean up fonts`. The explicit method exists because the reload path is internal (the base
painter calls `StyleSheetChanged` while preparing a frame, `MapPainter.cpp:2352-2354`), so without it
the release could be observed only by its effect on the next frame, and because a client may want to
release the fonts of a style switch it already knows about. This mirrors Cairo's stylesheet release
(`MapPainterCairo.cpp:937-971`) and adds the caller-visible point it lacks. No bound is introduced.

Alternatives:

- **(B)** Bounded retention with the least-recently-used eviction the measurement cache uses
  (`openspec/specs/map-painter-label-reuse/spec.md`). Rejected: the retained set already follows the
  pixel grid after decision 1, so the bound wins no memory; and the eviction rule releases the font the
  painter has not drawn with for the longest time, which is exactly the font a later frame may need
  again — the frame then resolves it anew and the cache thrashes. The measurement cache needs its bound
  because its key carries the label text, which is unbounded; a font identity has no such dimension.
- **(C)** Release in the destructor only, as SVG and IOS do today, and keep the reload release internal.
  Rejected: an application that switches stylesheets keeps the fonts of every stylesheet it ever loaded,
  and the contract would have no observable release point for a test.
- **(D)** Release everything whenever the drawing environment changes (font name, DPI, device). Rejected
  as a rule: the identity already carries the name and the size, so entries cannot go stale; a stylesheet
  reload is a genuine release point and is the one kept, because a reloaded stylesheet can rename or drop
  the styles whose fonts were resolved.

Risk of (A): a release discards fonts a later frame of the same view would still use, so that frame
resolves them again. Mitigation: the release happens on a stylesheet reload and at destruction, not per
frame, and the `resolved fonts are released` scenarios assert that the frame after the release draws as
the frame before it did.

### 3. The rule and the release live in the Qt painter, not in a shared helper

**Chosen (A):** implement both in `MapPainterQt`, next to the cache they govern.

Alternatives:

- **(B)** A shared bounded-or-released font-cache type in `libosmscout-map`, reused by Cairo, SVG, IOS
  and DirectX later. Rejected for now: the retained value is backend-specific (`QFont`, `CairoFont`, a
  face pointer, an `IDWriteTextFormat`), so the type would be a template over value and identity, and
  its first two adopters are files of an open pull request; extracting it before a second backend adopts
  it designs against a single case.
- **(C)** Reuse the label layouter's measurement cache machinery for fonts. Rejected: that cache is keyed
  by a measurement key and holds measurements; a font is neither, and the layouter lives in the shared
  map library.

Risk of (A): the follow-ups duplicate the pattern. Mitigation: recorded as follow-ups; the
`painter-font-cache` capability is the place the second adopter starts from, and the extraction can
happen when it lands.

### 4. The painter exposes what it resolved and what it retains

**Chosen (A):** `MapPainterQt` gains a counter of fonts resolved rather than served, in the shape of
`MapPainterCairo::GetResolvedFontCount()`, and a reader for the number of fonts it currently retains.
The retained count is what makes a release assertable: after a stylesheet reload it has to be zero.

Alternatives:

- **(B)** Assert the identity rule and the release through process memory or an allocator hook. Rejected:
  flaky across platforms and allocators; the repository's cache tests assert counts, not bytes.
- **(C)** A test-only subclass or friend access. Rejected: no precedent in this repository, and it would
  not let a client observe the cache either.

Risk of (A): more public API on the backend. Mitigation: both readers are const diagnostics, the Cairo
painter already exposes exactly this shape, and the API is documented where it is declared.

### Sequence: two labels in one pixel step, then a stylesheet reload

```
  frame N                                    frame N (second label)          stylesheet reload
  -------                                    ----------------------          -----------------
  label(size s1)                             label(size s2)                  StyleSheetChanged
      |                                          |                                |
      v                                          v                                v
  GetFont(projection,params,s1)               GetFont(...s2)                 fonts.clear()
      |                                          |                          fontLru.clear()
      v                                          v                                |
  identity1 = pixelGrid(s1*factor)            identity2 = pixelGrid(s2*factor)  |
      |                                          |   s2 differs from s1 by        |
      v                                          v   less than a device pixel    |
  fonts.contains(identity1)?  -- no --> resolve   |
      |                                    |       |
      v                                    v       v
  fonts.insert(identity1, font)          identity2 == identity1 ? -------> serve, no resolve
      |                                                            |
      v                                                     (else resolve, as above)
  draw label                                                  draw label            next frame: 0 retained
```

## Risks / Trade-offs

- [The identity resolution is too coarse for the measurement] → the identity is integral device pixels,
  which is today's behaviour, and the measurement and the drawing both go through the resolved font; the
  `measurement and drawn font agree` scenario pins it.
- [A stylesheet reload releases fonts a frame still draws with] → the reload is a deliberate release
  point, the next frame resolves what it needs, and the release scenario asserts that the frame after the
  release draws as the frame before it did.
- [The counter's meaning drifts from what tests assert] → `GetResolvedFontCount` counts resolutions, not
  cache entries, and one scenario asserts that a repeated frame leaves it unchanged.
- [The other backends' quantization changes rendered output] → out of scope here; the follow-up changes
  the drawn size by less than a device pixel, and the entry has to state that the Cairo and SVG paths
  currently draw fractional font sizes.
- [Verification needs a display] → cases run with `QT_QPA_PLATFORM=offscreen`, as AGENTS.md documents for
  Qt tests, and the CI Qt jobs already run tests that way.

## Migration Plan

Additive only: no format, database or public-contract break. Two const readers and the release are new;
existing calls are unaffected. Rollback is a revert — the cache returns to today's behaviour, with no
persisted state to migrate.

## Open Questions

- Whether the Qt client should call the stylesheet release on a style switch itself (the painter's
  measurement and font caches across a style switch are recorded as TODO §53). Deferrable: it does not
  change the requirements, the approach or the tasks; this change makes the painter's release exist and
  be asserted, and a client-side call is a separate decision.
