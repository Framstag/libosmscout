# Skia Renderer

## Purpose

Provide a `MapPainterSkia` class implementing the `osmscout::MapPainter` interface using the Skia 2D graphics library for hardware-accelerated map rendering.

## Requirements

### Requirement: MapPainterSkia class
The system SHALL provide a `MapPainterSkia` class that inherits from `osmscout::MapPainter` and implements all pure virtual methods.

#### Scenario: Class compiles and links
- **WHEN** building `libosmscout-map-skia` with a C++20 compiler
- **THEN** `MapPainterSkia` compiles without errors and links into `libosmscout_map_skia`

#### Scenario: DrawMap accepts SkCanvas pointer
- **WHEN** calling `MapPainterSkia::DrawMap(projection, parameter, data, canvas)`
- **THEN** the method accepts an `SkCanvas*` as the drawing target and returns `true`

### Requirement: Ground rendering
The system SHALL render the ground (background) using a solid fill color from the style.

#### Scenario: DrawGround fills entire canvas
- **WHEN** `DrawGround` is called with a `FillStyle` having color `#E0E0E0`
- **THEN** every pixel in the canvas is set to `RGB(224, 224, 224)`

### Requirement: Area rendering with solid fill
The system SHALL render polygon areas using a solid fill color from the `FillStyle`.

#### Scenario: DrawArea fills polygon
- **WHEN** `DrawArea` is called with an `AreaData` containing a rectangular polygon and a `FillStyle` with color `#FF0000`
- **THEN** pixels inside the polygon are set to `RGB(255, 0, 0)` and pixels outside are unchanged

### Requirement: Path rendering with solid stroke
The system SHALL render paths (ways) using a solid stroke color and width.

#### Scenario: DrawPath renders solid line
- **WHEN** `DrawPath` is called with a `Color(255,0,0)`, width `2.0`, and a straight line segment
- **THEN** pixels along the line are set to `RGB(255, 0, 0)` with the specified width

### Requirement: Stub implementations for non-essential methods
The system SHALL provide stub implementations for `HasIcon`, `GetFontHeight`, `RegisterRegularLabel`, `RegisterContourLabel`, `DrawLabels`, `DrawIcon`, `DrawSymbol`, and `DrawContourSymbol` that compile and do not crash at runtime.

#### Scenario: HasIcon returns false
- **WHEN** `HasIcon` is called with any style
- **THEN** it returns `false`

#### Scenario: GetFontHeight returns constant
- **WHEN** `GetFontHeight` is called with any projection, parameter, and fontSize
- **THEN** it returns a positive `double` value (e.g., `12.0`)

#### Scenario: Label methods do not crash
- **WHEN** `RegisterRegularLabel`, `RegisterContourLabel`, or `DrawLabels` are called
- **THEN** they return without modifying the canvas or crashing

### Requirement: Export macro for DLL visibility
The system SHALL provide `OSMSCOUT_MAP_SKIA_API` export/import macro for shared library builds.

#### Scenario: Export macro defined
- **WHEN** inspecting `MapSkiaImportExport.h`
- **THEN** `OSMSCOUT_MAP_SKIA_API` is defined as `__declspec(dllexport)` on Windows shared builds, `__declspec(dllimport)` on Windows static/consumer builds, and `__attribute__((visibility("default")))` on GCC/Clang shared builds
