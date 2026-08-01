## Context

The Skia backend (`libosmscout-map-skia`) was recently created as a minimal map renderer. It supports solid-color line strokes and area fills but lacks many style features that the Cairo and Qt backends implement. These features are required by standard OSM stylesheets (`.oss` files in `stylesheets/`).

Current state of `MapPainterSkia`:

- **`DrawPath()`**: Supports color, width, dash array. Ignores `startCap`/`endCap` (hardcodes `kRound_Cap`). No alpha. No gap color.
- **`DrawArea()`**: Supports fill color, border color, border width. No border dashes, no border gap color, no alpha, no clippings.
- **`DrawFillStyle()`**: Empty stub — no pattern fills, no combined fill+border rendering.
- **`DrawGround()`**: Uses `SkColorSetRGB` (no alpha).

The base class `MapPainter::DrawWay()` already handles the two-pass gap-color logic — it calls `DrawPath()` twice (gap color solid, then dashed line on top). The backend just needs to render each call correctly.

## Goals / Non-Goals

**Goals:**

- Add 7 capabilities from the proposal: line gap color, line cap styles, line alpha, border dash patterns, border gap color, area clippings, pattern fills
- Match visual output of Cairo/Qt backends for the same `.oss` stylesheets
- Keep API surface unchanged — only implement existing virtual methods

**Non-Goals:**

- No new public API or style types
- No SVG pattern support (only PNG, matching Cairo's current implementation)
- No contour symbol rendering (separate concern)
- No label rendering improvements (separate concern)
- No performance optimization beyond reasonable implementation

## Decisions

### Decision 1: Private `SetLineAttributes()` helper

**Chosen:** Add a private `SetLineAttributes(SkPaint&, const Color&, double width, const std::vector<double>& dash, LineStyle::CapStyle startCap, LineStyle::CapStyle endCap)` method.

**Rationale:** Both `DrawPath()` and `DrawArea()` border rendering need identical paint setup logic (color with alpha, width, dash, cap styles). A helper avoids duplication. Cairo uses the same pattern (`SetLineAttributes`).

**Alternatives considered:**
- Inline in each method — rejected because it duplicates ~15 lines across 3+ call sites
- Lambda inside each method — rejected because the helper is also needed for the gap-color two-pass pattern

### Decision 2: Pattern fill cache using `std::map<std::string, sk_sp<SkShader>>`

**Chosen:** Cache loaded pattern shaders keyed by pattern filename, stored as `std::map<std::string, sk_sp<SkShader>>` member.

**Rationale:** Patterns are loaded from PNG files on disk. Caching avoids reloading on every frame. Cairo uses a vector indexed by pattern ID; Skia shaders are immutable and cheap to store as `sk_sp`.

**Alternatives considered:**
- Vector indexed by `patternId` (matching Cairo) — rejected because Skia doesn't have a separate surface/pattern split; shader is the final object
- Load every frame — rejected for performance

### Decision 3: Area clippings via `SkPath::setFillType(kEvenOdd)`

**Chosen:** Add clipping sub-paths to the same `SkPath` and set fill type to `kEvenOdd`.

**Rationale:** This matches Cairo's `CAIRO_FILL_RULE_EVEN_ODD` approach exactly. Qt also adds sub-paths. Skia natively supports even-odd fill, making this straightforward.

**Alternatives considered:**
- `SkClipPath` with `SkClipOp::kDifference` per clipping region — more complex, requires save/restore, no benefit over even-odd
- Separate path for each ring — would require manual winding detection

### Decision 4: Gap color for borders handled in `DrawArea()`, not `DrawFillStyle()`

**Chosen:** Implement border gap color directly in `DrawArea()` using the same two-pass pattern as `DrawWay()`: draw solid gap color border first, then dashed border on top.

**Rationale:** The base class `MapPainter` does not provide a two-pass border abstraction. `DrawArea()` receives the full `AreaData` with `borderStyle`, so it can inspect `borderStyle->GetGapColor()` and `borderStyle->HasDashes()` directly. Cairo's `DrawFillStyle()` handles this for borders, but Skia's `DrawFillStyle()` is a separate call — keeping the logic in `DrawArea()` is simpler.

**Alternatives considered:**
- Implement in `DrawFillStyle()` matching Cairo — rejected because `DrawFillStyle()` is called from `DrawArea()` in Cairo's flow, but Skia's `DrawArea()` currently handles fill+border inline. Restructuring to match Cairo's flow would be a larger refactor.

## Risks / Trade-offs

- **Pattern loading failure** → If a PNG pattern file is missing, fall back to solid fill color (matching Cairo behavior: `HasPattern()` returns false, solid color used instead)
- **Alpha on ground** → `DrawGround()` currently ignores alpha. Changing to `SkColorSetARGB` could change ground tile appearance if alpha < 1. Low risk since ground fill color alpha is typically 1.0 in stylesheets.
- **Cap style mismatch** → Skia's `kSquare_Cap` extends half the stroke width beyond endpoints, matching Cairo's `CAIRO_LINE_CAP_SQUARE`. Qt's `SquareCap` behaves the same. No risk.
- **Border width below minimum** → `DrawArea()` should skip border if `borderWidth < parameter.GetLineMinWidthPixel()`, matching Cairo's check in `DrawFillStyle()`.

## Open Questions

- Should `DrawFillStyle()` be fully implemented (matching Cairo) or left as a stub? Current plan: implement pattern fills only; border rendering stays in `DrawArea()`.
- Pattern scaling: Cairo applies patterns at 1:1 pixel scale. Should Skia match this exactly, or support DPI-aware scaling? Match Cairo for consistency.
