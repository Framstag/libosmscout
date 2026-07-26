# javascout-auto-zoom

## Purpose

TBD

## Requirements

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
- **WHEN** the breakpoints are (15 km/h → mag 15) and (30 km/h → mag 14)
- **THEN** the computed magnification is approximately 14.5 (linearly interpolated)

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
- **THEN** auto-zoom uses a default speed of 20 km/h to compute a reasonable initial zoom
- **AND** the magnification jumps directly to the target instead of smoothing from the default map zoom

### Requirement: Smooth zoom transitions
The system SHALL move the magnification toward the target at most 1 level per position update, using fractional `double` values for smooth convergence.

#### Scenario: Speed changes abruptly after tunnel
- **GIVEN** the vehicle exits a tunnel and speed jumps from unknown to 60 km/h
- **WHEN** position estimates arrive at ~1/sec
- **THEN** the magnification changes by at most 1 level per update toward the target
- **AND** the transition takes multiple seconds instead of happening instantly

### Requirement: Speed spike rejection
The system SHALL reject speed values exceeding 150 km/h and use the last known good speed instead.

#### Scenario: SpeedAgent reports bogus 392 km/h after GPS gap
- **GIVEN** the SpeedAgent computes a spuriously high speed (e.g. 392 km/h) after a tunnel gap
- **WHEN** `onCurrentSpeed` delivers this value
- **THEN** the auto-zoom logic SHALL use the last good speed (≤ 150 km/h) instead
- **AND** the zoom level SHALL NOT jump to the bogus speed's target

### Requirement: Turn-aware zoom boosting
The system SHALL boost the target magnification when approaching a turn, and hold the boost until 600m past the turn.

#### Scenario: Approaching a turn at 300m
- **GIVEN** the vehicle is 300m from the next turn
- **WHEN** a position estimate arrives
- **THEN** the target magnification SHALL be at least 16.0
- **AND** the zoom SHALL remain boosted until 600m past the turn

#### Scenario: Between 300m and 600m from turn
- **GIVEN** the vehicle is between 300m and 600m from the next turn
- **WHEN** a position estimate arrives
- **THEN** the target magnification SHALL be at least 15.0

### Requirement: Initial zoom uses routing-sensible default
The system SHALL initialize the current magnification to 15.0 instead of the default map magnification (5) when navigation is active.

#### Scenario: Navigation starts
- **GIVEN** the user starts navigation
- **WHEN** the first position estimate arrives
- **THEN** the initial zoom SHALL be approximately 15.0 (routing-sensible)
- **AND** the map SHALL NOT start at zoom level 5 (very zoomed out)

### Requirement: SPEED_ZOOM_TABLE with narrowed range
The system SHALL use a speed-to-magnification table with a range of 17→13 (4 levels), with linear interpolation between breakpoints.

#### Scenario: Speed of 100 km/h
- **GIVEN** the vehicle is driving at 100 km/h
- **WHEN** the auto-zoom computes the target magnification
- **THEN** the target SHALL be approximately 14.0

#### Scenario: Speed of 5 km/h
- **GIVEN** the vehicle is walking at 5 km/h
- **WHEN** the auto-zoom computes the target magnification
- **THEN** the target SHALL be approximately 16.5
