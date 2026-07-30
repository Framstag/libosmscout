# map-painter-skia

## Purpose

The Skia map backend (`libosmscout-map-skia`) provides map rendering using the Skia graphics library. This spec covers the `MapPainterSkia` class, its rendering methods, cache management, and lifecycle callbacks.

## Requirements

### Requirement: StyleSheetChanged clears icon and pattern caches

The `MapPainterSkia` class SHALL override `StyleSheetChanged` to clear icon and pattern caches when the style sheet changes, preventing stale visual artifacts from the previous style.

#### Scenario: StyleSheetChanged clears iconCache

- **WHEN** `MapPainterSkia::StyleSheetChanged` is called
- **THEN** `iconCache` (`std::map<std::string, sk_sp<SkImage>>`) SHALL be empty

#### Scenario: StyleSheetChanged clears patternCache

- **WHEN** `MapPainterSkia::StyleSheetChanged` is called
- **THEN** `patternCache` (`std::map<std::string, sk_sp<SkShader>>`) SHALL be empty

#### Scenario: StyleSheetChanged matches Cairo/Qt behavior

- **WHEN** `MapPainterSkia::StyleSheetChanged` is called
- **THEN** it SHALL clear cached rendering resources without performing additional drawing operations (matching the pattern in `MapPainterCairo::StyleSheetChanged` and `MapPainterQt::StyleSheetChanged`)
