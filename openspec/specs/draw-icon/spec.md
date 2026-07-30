# draw-icon

## Purpose

The Skia map backend SHALL implement `MapPainterSkia::DrawIcon` and `MapPainterSkia::HasIcon` to load, cache, and render PNG icon images from configured icon paths.

## Requirements

### Requirement: MapPainterSkia::DrawIcon renders PNG icons

The system SHALL implement `MapPainterSkia::DrawIcon` to load PNG icon images from configured icon paths, cache them, and render them scaled to the requested dimensions.

#### Scenario: DrawIcon loads and caches PNG icon

- **WHEN** `DrawIcon` is called with an `IconStyle` that has a valid icon name
- **THEN** the system SHALL search the configured icon paths for `<icon-name>.png`
- **THEN** the loaded image SHALL be cached for subsequent calls with the same icon name
- **THEN** the icon SHALL be drawn centered at the specified position, scaled to the specified width and height

#### Scenario: DrawIcon uses cached image on subsequent calls

- **WHEN** `DrawIcon` is called twice with the same icon name
- **THEN** the image SHALL be loaded from disk only on the first call
- **THEN** the second call SHALL use the cached `sk_sp<SkImage>`

#### Scenario: DrawIcon handles missing icon file

- **WHEN** `DrawIcon` is called with an icon name that has no matching PNG file in any icon path
- **THEN** nothing SHALL be drawn and no error SHALL occur

### Requirement: MapPainterSkia::HasIcon detects and configures icon availability

The system SHALL implement `MapPainterSkia::HasIcon` to check if an icon is available, set up its dimensions based on the icon mode, and return availability status.

#### Scenario: HasIcon returns true for available icon

- **WHEN** `HasIcon` is called with an `IconStyle` whose icon PNG exists in the configured paths
- **THEN** it SHALL return `true`
- **THEN** the icon width and height SHALL be set according to the icon mode:
  - For `Scalable` or `ScaledPixmap` mode: use `parameter.GetIconSize()` converted to pixels
  - For `OriginalPixmap` mode: use the actual image pixel dimensions

#### Scenario: HasIcon returns false for missing icon

- **WHEN** `HasIcon` is called with an `IconStyle` whose icon PNG does not exist
- **THEN** it SHALL return `false`

#### Scenario: HasIcon returns false for already-failed icon

- **WHEN** `HasIcon` is called with an `IconStyle` that previously failed to load
- **THEN** it SHALL return `false` without retrying the disk lookup
