# javascout-auto-zoom

## ADDED Requirements

### Requirement: Map magnification adjusts automatically based on navigation speed
The system SHALL adjust the map magnification level based on the current vehicle speed when follow mode and auto-zoom are both active.

#### Scenario: Speed increases during navigation
- **GIVEN** an active navigation session with follow mode and auto-zoom enabled
- **WHEN** the vehicle speed increases (e.g. from city driving to highway)
- **THEN** the map magnification decreases (zooms out) according to the speed-to-magnification mapping
- **AND** the map remains centered on the vehicle position

#### Scenario: Speed decreases during navigation
- **GIVEN** an active navigation session with follow mode and auto-zoom enabled
- **WHEN** the vehicle speed decreases (e.g. from highway to city driving)
- **THEN** the map magnification increases (zooms in) according to the speed-to-magnification mapping
- **AND** the map remains centered on the vehicle position

### Requirement: Auto-zoom uses linear interpolation between speed breakpoints
The system SHALL use a configurable lookup table of speed-to-magnification pairs with linear interpolation between entries to produce smooth zoom transitions.

#### Scenario: Speed between breakpoints
- **GIVEN** a speed of 22 km/h
- **WHEN** the breakpoints are (15 km/h → mag 16) and (30 km/h → mag 15)
- **THEN** the computed magnification is approximately 15.5 (linearly interpolated)

### Requirement: Manual zoom temporarily suspends auto-zoom
The system SHALL suspend auto-zoom when the user manually changes the zoom level, and re-engage it when the speed crosses a threshold boundary.

#### Scenario: User zooms in manually
- **GIVEN** auto-zoom is active at mag 14 (highway speed)
- **WHEN** the user zooms in to mag 16 to inspect a junction
- **THEN** auto-zoom is suspended
- **AND** the map stays at mag 16 even as speed changes

#### Scenario: Speed crosses threshold boundary
- **GIVEN** auto-zoom is suspended after a manual zoom
- **WHEN** the speed changes from highway (90 km/h) to city (30 km/h), crossing a table boundary
- **THEN** auto-zoom re-engages
- **AND** the magnification adjusts to the speed-appropriate level

### Requirement: Auto-zoom can be toggled on and off
The system SHALL provide a UI control to enable or disable auto-zoom independently of follow mode.

#### Scenario: Auto-zoom disabled
- **GIVEN** follow mode is active
- **WHEN** the user disables auto-zoom
- **THEN** the map magnification stays at the current level regardless of speed changes
- **AND** follow mode continues to re-center on the vehicle position

#### Scenario: Auto-zoom re-enabled
- **GIVEN** auto-zoom is disabled
- **WHEN** the user re-enables auto-zoom
- **THEN** the magnification immediately adjusts to the speed-appropriate level
- **AND** auto-zoom suspension state is reset

### Requirement: Auto-zoom uses the navigation engine's reported speed
The system SHALL use the speed value from the `onCurrentSpeed(double speedKmH)` callback as the input for zoom calculation, not raw GPS position deltas.

#### Scenario: Speed unknown
- **GIVEN** the navigation engine has not yet reported a speed (speed is negative)
- **WHEN** a position estimate arrives
- **THEN** auto-zoom uses the last known speed
- **AND** if no speed has ever been reported, the magnification stays unchanged
