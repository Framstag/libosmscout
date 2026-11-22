# Proposal

## Why

A map painter that is kept open for the lifetime of an application never gives a resolved font back:
the Qt painter's cache is released nowhere — its destructor carries a `// TODO: Clean up fonts` — and
the stylesheet reload path next to it already drops the pattern images of the replaced stylesheet
without dropping the fonts. The identity an entry is selected by is also not a rule the code states:
the size a cached font is selected by is the product of the label's size with the projection's width
factor, and in the Qt painter that product reaches the key through a narrowing conversion into an
integral field, so the identity is the truncation of that product by accident of the field's type.
Nothing pins it: a change of the key's declared type would silently change which labels share a font,
and the number of fonts a session holds would follow the number of labels rather than what is drawn
(the Cairo and SVG painters, whose key keeps the product as a floating-point value, resolve a font per
label for that reason). This was recorded as the residual of the Cairo and SVG font-cache work
(`fix-font-cache-key`, which fixed the name half and left this out of scope).

## What Changes

- The identity a painter selects a cached resolved font by SHALL follow from the drawing parameters at
  the resolution of the drawing: two labels whose resolved font sizes differ by less than a device
  pixel SHALL share one font, so the number of fonts a session resolves follows what is drawn on
  screen and not the number of labels rendered.
- The resolved fonts a painter retains SHALL be released when a stylesheet reload replaces the
  stylesheet they were resolved for, and when the painter is destroyed.
- The Qt backend's cache is brought in line with that contract: the identity rule is stated where the
  key is declared, the truncation becomes an explicit operation, and the release is added; the
  `// TODO: Clean up fonts` there is closed.
- The number of fonts a painter resolved and the number it retains are observable, so the contract can
  be asserted by a test rather than by a memory measurement.

Explicitly not part of this change: **no cache bound.** A bound that evicts the least recently drawn
font can release a font a later frame still draws with, so the frame that needs it resolves it again —
the cache thrashes, which is the opposite of what it is for. The measurement cache needs its bound
because its key carries the label text, which has no limit; a font identity carries only the name, the
weight/style and the size, and once the size is quantized to the drawing's pixel grid there is no
unbounded dimension left for a bound to manage.

Out of scope (recorded follow-ups): the Cairo, SVG, IOS and DirectX caches resolve their size from an
unquantized value or from a size alone, and their release points differ (Cairo releases on a stylesheet
reload, SVG and IOS only on destruction, DirectX on destruction and device loss). Quantizing their
identity to the pixel grid changes the size they draw at by less than a device pixel — a rendering
change that has to be reviewed on a backend that draws fractional font sizes, and their files are
currently owned by an open pull request.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `painter-font-cache`: the contract for the resolved-font cache a map painter keeps across frames.
  The capability, introduced by the in-flight change `fix-font-cache-key`, today states only which
  inputs a cache entry depends on. It gains the requirements that the identity of an entry follows the
  drawing parameters at the resolution of the drawing, that the retained fonts are released when the
  stylesheet they were resolved for is replaced or the painter is destroyed, that the release does not
  change the rendered output, and that the resolved and the retained counts are observable.

## Impact

Affected code:

- `libosmscout-map-qt/include/osmscoutmapqt/MapPainterQt.h` — the resolved-font cache and its identity
  type (`FontDescriptor`, `QMap<FontDescriptor,QFont> fonts`), the destruction path and the diagnostic
  the tests assert on.
- `libosmscout-map-qt/src/osmscoutmapqt/MapPainterQt.cpp` — `GetFont`, which builds the identity from
  the requested font name and the scaled font size, `StyleSheetChanged`, which releases the frame state
  of a replaced stylesheet, and the destructor's `// TODO: Clean up fonts`.

Tests and build:

- `Tests/src/TextMetricsQtTest.cpp` — cases for the identity rule, the release and the unchanged output
  after a release.
- If the cases outgrow the existing target: `Tests/CMakeLists.txt`, `Tests/meson.build`.

Not affected: label measurement reuse, font-name resolution (fixed by `fix-font-cache-key`), the
stylesheets, and the other backends listed under out of scope.
