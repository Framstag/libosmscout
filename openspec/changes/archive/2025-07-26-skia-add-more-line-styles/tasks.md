## 1. Foundation — Color alpha and helper extraction

- [x] 1.1 Add `SetLineAttributes()` private helper to `MapPainterSkia` that configures a `SkPaint` with RGBA color, width, dash pattern, and cap styles
- [x] 1.2 Update all `SkColorSetRGB` calls in `MapPainterSkia.cpp` to `SkColorSetARGB` — affects `DrawPath()`, `DrawArea()` fill, `DrawArea()` border, `DrawGround()`
- [x] 1.3 Verify build compiles with no warnings

## 2. Line cap styles

- [x] 2.1 Map `LineStyle::CapStyle` values to `SkPaint::Cap` in `DrawPath()`: `capButt` → `kButt_Cap`, `capRound` → `kRound_Cap`, `capSquare` → `kSquare_Cap`
- [x] 2.2 Verify existing tests still pass

## 3. Line gap color

- [x] 3.1 In `DrawPath()`, when `dash` is non-empty and gap color is visible, draw solid gap-color stroke before the dashed stroke
- [x] 3.2 Verify existing tests still pass

## 4. Border dash patterns

- [x] 4.1 In `DrawArea()` border rendering, apply `borderStyle->GetDash()` via `SkDashPathEffect` when non-empty
- [x] 4.2 Verify existing tests still pass

## 5. Border gap color

- [x] 5.1 In `DrawArea()` border rendering, when `borderStyle->HasDashes()` and `borderStyle->GetGapColor().IsVisible()`, draw solid gap-color border before dashed border
- [x] 5.2 Verify existing tests still pass

## 6. Area clippings

- [x] 6.1 In `DrawArea()`, add clipping sub-paths from `area.clippings` to the main `SkPath` and set `SkPathFillType::kEvenOdd`
- [x] 6.2 Verify existing tests still pass

## 7. Pattern fills

- [x] 7.1 Add pattern image cache member (`std::map<std::string, sk_sp<SkShader>>`) to `MapPainterSkia`
- [x] 7.2 Implement `HasPattern()` logic: load PNG from pattern paths, create `SkShader` with `SkTileMode::kRepeat`, cache by filename
- [x] 7.3 Implement `DrawFillStyle()`: handle pattern fill (shader), solid fill, and combined fill+border (fill preserve then stroke)
- [x] 7.4 Verify build compiles with no warnings

## 8. Build and test verification

- [x] 8.1 Build with CMake (`-DOSMSCOUT_BUILD_MAP_SKIA=ON`) and verify no errors
- [x] 8.2 Build with Meson and verify no errors
- [x] 8.3 Run full test suite and verify no regressions

## 9. Unit tests

- [x] 9.1 Add unit tests for cap style mapping, alpha transparency, dash patterns, clipping path construction, and pixel-level rendering
