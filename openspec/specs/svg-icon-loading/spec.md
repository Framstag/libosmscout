# SVG Icon Loading

## Purpose

Support loading and rendering of SVG format icons in the Skia map backend, with Skia's SkSVGDOM as primary renderer and nanosvg as fallback.

## Requirements

### Requirement: Load SVG icons from stylesheet paths

The system SHALL load SVG format icons referenced by `.oss` stylesheets from the configured icon search paths.

The system SHALL probe for `.svg` extension after failing to find a `.png` file with the same icon name.

The system SHALL support both `.svg` and `.png` icons coexisting in the same stylesheet.

#### Scenario: SVG icon found and loaded

- **WHEN** a stylesheet references icon `amenity_pub` and a file `amenity_pub.svg` exists in an icon search path
- **THEN** the system SHALL parse the SVG file and cache the rendered result

#### Scenario: PNG icon takes precedence

- **WHEN** both `amenity_pub.png` and `amenity_pub.svg` exist in the same icon search path
- **THEN** the system SHALL load the PNG file and skip the SVG file

#### Scenario: SVG icon not found

- **WHEN** no `.svg` file exists for the referenced icon name
- **THEN** the system SHALL mark the icon as not found (same behavior as missing PNG)

### Requirement: Render SVG icons to raster

The system SHALL rasterize SVG icons to pixel buffers for display.

The system SHALL use Skia's `SkSVGDOM` module as the primary SVG renderer when available at build time.

The system SHALL use nanosvg as the fallback SVG renderer when the Skia SVG module is not available.

The system SHALL cache the rasterized result as an `SkImage` in the icon cache.

#### Scenario: SkSVGDOM renders SVG

- **WHEN** the Skia SVG module is available and a valid SVG file is loaded
- **THEN** the system SHALL render the SVG to an offscreen `SkSurface` and snapshot to `SkImage`

#### Scenario: nanosvg renders SVG as fallback

- **WHEN** the Skia SVG module is not available and a valid SVG file is loaded
- **THEN** the system SHALL parse and rasterize the SVG via nanosvg and wrap the result as `SkImage`

#### Scenario: Malformed SVG is handled gracefully

- **WHEN** an SVG file contains invalid XML or unsupported elements
- **THEN** the system SHALL NOT crash and SHALL mark the icon as not found

### Requirement: Support icon scaling modes

The system SHALL respect the configured `IconMode` when rendering SVG icons.

#### Scenario: Scalable mode

- **WHEN** `IconMode::Scalable` or `IconMode::ScaledPixmap` is active
- **THEN** the system SHALL scale the SVG to the configured icon pixel size

#### Scenario: Original pixmap mode

- **WHEN** `IconMode::OriginalPixmap` is active
- **THEN** the system SHALL use the SVG's native dimensions

### Requirement: Cache invalidation on style change

SVG icons cached in `iconCache` SHALL be cleared when `StyleSheetChanged` is called, matching existing PNG cache behavior.

#### Scenario: Style change clears SVG cache

- **WHEN** a style sheet change triggers `StyleSheetChanged`
- **THEN** all cached SVG icons SHALL be removed from `iconCache`
