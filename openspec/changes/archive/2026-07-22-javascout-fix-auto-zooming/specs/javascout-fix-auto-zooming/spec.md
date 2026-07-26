# javascout-fix-auto-zooming

## Purpose

Fix auto-zoom by speed in JavaScout: prevent jarring zoom jumps on unknown speed, smooth transitions, reject bogus SpeedAgent values after GPS gaps, and add turn-aware zoom boosting.

## ADDED Requirements

### Requirement: Auto-zoom skips unknown speed
The system SHALL NOT apply auto-zoom when the current speed is unknown (`lastSpeedKmH < 0`). The zoom level SHALL remain at its current value until a valid speed is reported.

#### Scenario: Navigation starts with unknown speed
- **GIVEN** navigation has just started and no speed has been reported yet
- **WHEN** the first position estimate arrives
- **THEN** the zoom level stays at the current value (or jumps directly to the default-speed target)
- **AND** no jarring jump to mag 18 occurs

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

### Requirement: SpeedAgent FIFO reset on GPS gap
The SpeedAgent SHALL clear its speed FIFO when the time gap between consecutive GPS updates exceeds 10 seconds.

#### Scenario: GPS signal lost in tunnel
- **GIVEN** the GPS signal is lost for 103 seconds in a tunnel
- **WHEN** the signal is reacquired
- **THEN** the SpeedAgent SHALL clear its FIFO before adding the new segment
- **AND** the speed SHALL be computed only from post-reacquisition segments

### Requirement: SpeedAgent speed sanity cap
The SpeedAgent SHALL report `-1.0` (unknown) when the computed speed exceeds 200 km/h.

#### Scenario: Bogus speed from position jump
- **GIVEN** the SpeedAgent computes a speed of 392 km/h from a position jump
- **WHEN** the speed exceeds 200 km/h
- **THEN** the SpeedAgent SHALL report `-1.0` instead
- **AND** the Java side SHALL fall back to the last good speed

### Requirement: Initial zoom uses routing-sensible default
The system SHALL initialize `currentSmoothMag` to 15.0 instead of the default map magnification (5).

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
