# Area Clippings

**Purpose:** The Skia backend supports `AreaData::clippings` (interior holes) using the even-odd fill rule.

## Requirements

### Requirement: Area clipping support

The Skia backend SHALL support `AreaData::clippings` (interior holes) using even-odd fill rule.

#### Scenario: Area with clippings renders holes
- **WHEN** `DrawArea()` is called with non-empty `area.clippings`
- **THEN** the area path SHALL include clipping sub-paths and use `SkPathFillType::kEvenOdd`

#### Scenario: Area without clippings renders normally
- **WHEN** `DrawArea()` is called with empty `area.clippings`
- **THEN** the area path SHALL use the default fill type (no change from current behavior)
