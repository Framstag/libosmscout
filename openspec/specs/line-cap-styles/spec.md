# Line Cap Styles

## Purpose

The Skia backend honors `LineStyle::CapStyle` values (`capButt`, `capRound`, `capSquare`) for both `joinCap` and `endCap` in `DrawPath()`.

## Requirements

### Requirement: Line cap style support

The Skia backend SHALL honor `LineStyle::CapStyle` values (`capButt`, `capRound`, `capSquare`) for both `joinCap` and `endCap` in `DrawPath()`.

#### Scenario: Butt cap renders flat ends
- **WHEN** `DrawPath()` is called with `startCap == LineStyle::capButt` or `endCap == LineStyle::capButt`
- **THEN** the corresponding line end uses `SkPaint::kButt_Cap`

#### Scenario: Round cap renders rounded ends
- **WHEN** `DrawPath()` is called with `startCap == LineStyle::capRound` or `endCap == LineStyle::capRound`
- **THEN** the corresponding line end uses `SkPaint::kRound_Cap`

#### Scenario: Square cap renders square ends
- **WHEN** `DrawPath()` is called with `startCap == LineStyle::capSquare` or `endCap == LineStyle::capSquare`
- **THEN** the corresponding line end uses `SkPaint::kSquare_Cap`

#### Scenario: Default cap is round
- **WHEN** `DrawPath()` is called (any cap values)
- **THEN** the default/fallback cap style SHALL be `SkPaint::kRound_Cap`
