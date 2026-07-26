# Map Rotation

## Purpose

Define how the map projection rotates to align with the vehicle's driving direction during navigation, and how the rotation angle flows through the render pipeline.

## Requirements

### Requirement: Map rotates to driving direction during navigation

When navigation is active, follow-mode is on, and rotation mode is DRIVING_DIRECTION_UP, the map SHALL rotate so the vehicle's heading points toward the top of the screen.

#### Scenario: Map rotates on position estimate
- **WHEN** navigation is active, follow-mode is on, and rotation mode is DRIVING_DIRECTION_UP
- **AND** a position estimate with bearing 90° (east) is received
- **THEN** the map SHALL rotate so east is at the top of the viewport

#### Scenario: Map stays north-up when rotation mode is NORTH_UP
- **WHEN** navigation is active and rotation mode is NORTH_UP
- **THEN** the map SHALL remain with north at the top regardless of bearing

#### Scenario: Rotation pauses on manual interaction
- **WHEN** the user pans or zooms the map while driving-direction-up mode is active
- **THEN** the map SHALL stop rotating to the current bearing
- **AND** follow-mode SHALL be automatically disabled

#### Scenario: Rotation resumes when follow-mode re-engaged
- **WHEN** the user re-enables follow-mode while driving-direction-up mode is active
- **THEN** the map SHALL resume rotating to the vehicle bearing

### Requirement: Angle is passed through the full render pipeline

The `render()` and `renderWithRouteAndPois()` JNI methods SHALL accept a `double angle` parameter (radians, 0 = north-up). The C++ JNI bridge SHALL pass this angle to `MercatorProjection::Set()`.

#### Scenario: Render with non-zero angle
- **WHEN** `render()` is called with angle=1.5708 (90°)
- **THEN** the returned pixel data SHALL show the map rotated 90° clockwise

#### Scenario: Render with zero angle matches current behavior
- **WHEN** `render()` is called with angle=0.0
- **THEN** the returned pixel data SHALL be identical to the current (pre-change) output

### Requirement: projectToPixel() accepts angle

The `projectToPixel()` JNI method SHALL accept a `double angle` parameter so that coordinate-to-screen projection is correct when the map is rotated.

#### Scenario: Projection with rotation
- **WHEN** `projectToPixel()` is called with a known coordinate and a non-zero angle
- **THEN** the returned pixel coordinates SHALL reflect the rotated projection

### Requirement: Location marker draws at correct position on rotated map

The `drawCurrentLocationMarker()` method in MapRenderer SHALL use the rotated projection when computing screen coordinates for the location marker.

#### Scenario: Marker position on rotated map
- **WHEN** the map is rotated 90° and a location marker is at the map center
- **THEN** the marker SHALL be drawn at the center of the canvas
