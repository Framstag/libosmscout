# javascout-track-playback

## Purpose

TBD

## Requirements

### Requirement: Imported GPX tracks can drive simulated GPS fixes
The system SHALL replay an imported GPX track as a time-ordered sequence of GPS fixes supplied to the navigation controller.

#### Scenario: Start playback of imported track
- **WHEN** the user selects an imported track and starts playback
- **THEN** the JavaScout TrackPlayer reads TrackPoint timestamps and emits processLocation calls at the correct wall-clock intervals

### Requirement: Playback speed is configurable
The system SHALL support a speed multiplier applied to the recorded track timestamps.

#### Scenario: Change playback speed
- **WHEN** the user selects a speed multiplier of 2.0
- **THEN** the wall-clock interval between fixes is halved

#### Scenario: Default playback speed
- **WHEN** no speed multiplier is configured
- **THEN** the default multiplier is 1.0 and the track is replayed at recorded timing

### Requirement: Playback controls are available
The system SHALL provide play, pause, stop, and resume controls for track playback.

#### Scenario: Pause and resume playback
- **WHEN** the user pauses playback and later resumes
- **THEN** the TrackPlayer stops and restarts emitting fixes from the paused point

### Requirement: Playback ends cleanly
The system SHALL stop emitting fixes and report completion when the last track point is reached.

#### Scenario: Track ends
- **WHEN** the final TrackPoint has been processed
- **THEN** the TrackPlayer transitions to stopped state and notifies the UI
