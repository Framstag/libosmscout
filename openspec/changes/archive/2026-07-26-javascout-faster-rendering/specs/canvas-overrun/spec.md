## ADDED Requirements

### Requirement: Render larger area than viewport
The system SHALL render the map at 2.5× the visible screen dimensions into the off-screen buffer. When the user pans, the system SHALL always copy the overlapping sub-region from the buffer to the Canvas. Areas of the viewport that extend beyond the rendered buffer SHALL be filled with a background color. A full re-render SHALL be triggered asynchronously to fill the missing areas.

#### Scenario: Pan within overrun area uses sub-region blit
- **WHEN** the user pans the map within the rendered area
- **THEN** the system SHALL compute the pixel offset between old and new view center using the Mercator projection
- **AND** the system SHALL copy the relevant sub-region from the off-screen buffer to the Canvas
- **AND** no JNI render call SHALL be made

#### Scenario: Pan beyond overrun margin shows gray areas
- **WHEN** the user pans beyond the overrun margin (new viewport extends outside the rendered area)
- **THEN** the system SHALL fill the canvas with a background color
- **AND** the system SHALL copy the overlapping part of the front buffer (clipped to buffer bounds)
- **AND** the system SHALL trigger a full re-render asynchronously to fill the missing areas
- **AND** the map SHALL remain responsive during the re-render

#### Scenario: Overrun factor is configurable
- **WHEN** the overrun factor is changed from the default 2.5
- **THEN** the system SHALL use the new factor for subsequent renders
- **AND** the buffer SHALL be re-allocated to match

#### Scenario: Zoom scales current buffer as placeholder
- **WHEN** the user zooms in or out
- **THEN** the system SHALL scale the current front buffer to the new magnification level
- **AND** the system SHALL display the scaled buffer immediately (no delay)
- **AND** the marker SHALL be correctly positioned at the new zoom level
- **AND** the system SHALL trigger a high-quality full render
- **AND** the full render SHALL replace the placeholder when complete
- **AND** zooming SHALL feel responsive even with degraded visual quality

#### Scenario: Sub-region copy handles rotation
- **WHEN** the map is rotated and the user pans
- **THEN** the overrun buffer SHALL be invalidated (rotation changes projection)
- **AND** a full re-render SHALL be triggered at the new angle

#### Scenario: Zoom out scales buffer down
- **WHEN** the user zooms out
- **THEN** the system SHALL scale the front buffer down by `2^(newMag - oldMag)`
- **AND** the scaled buffer SHALL be centered on the canvas with background fill around it

#### Scenario: Overrun buffer is invalidated on zoom
- **WHEN** zoom level changes
- **THEN** the overrun buffer SHALL be invalidated
- **AND** a full re-render SHALL be triggered at the new magnification

### Requirement: Pixel offset uses Mercator projection
The system SHALL compute the sub-region pixel offset using the full Mercator projection (same formula used for marker positioning), not a flat-earth approximation. This ensures the map content and overlay markers stay aligned during pan.

#### Scenario: Offset matches marker projection
- **WHEN** the user pans the map
- **THEN** the pixel offset SHALL be computed by projecting both old and new centers through `geoToScreen()`
- **AND** the marker SHALL be drawn at the same projected position
- **AND** the marker SHALL not drift relative to map content during pan

#### Scenario: Window resize triggers re-render
- **WHEN** the window is resized (larger or smaller)
- **THEN** the Canvas dimensions SHALL be updated to match the parent panel
- **AND** the canvas width/height properties SHALL be unbound before setting (layout system may bind them)
- **AND** the parent panel SHALL have its minimum size set to 0 to allow shrinking below canvas size
- **AND** the panel SHALL have a clip rectangle bound to its dimensions to prevent canvas overflow
- **AND** a full re-render SHALL be triggered at the new canvas size
- **AND** the overrun buffer SHALL be re-allocated to match the new canvas dimensions × overrun factor
- **AND** the status bar SHALL remain visible at all window sizes
