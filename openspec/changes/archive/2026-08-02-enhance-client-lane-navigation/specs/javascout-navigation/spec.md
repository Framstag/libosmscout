## MODIFIED Requirements

### Requirement: Speed and lane information are reported

The system SHALL forward current speed, max allowed speed, and lane information from the engine to Java listeners. Lane information SHALL include the full per-lane turn array as `LaneTurn[]` values in addition to the summary turn string.

#### Scenario: Speed limit changes
- **WHEN** the engine emits a CurrentSpeedMessage or MaxAllowedSpeedMessage
- **THEN** the corresponding Java `NavigationListener` callback is invoked

#### Scenario: Lane update with full turn array
- **WHEN** the engine emits a LaneUpdateMessage with `turns=[LEFT, STRAIGHT_ON, RIGHT]`
- **THEN** the Java `NavigationListener.onLaneUpdate` callback is invoked with `turns` containing three `LaneTurn` values: `LEFT`, `STRAIGHT_ON`, `RIGHT`
- **AND** the `turn` parameter SHALL contain the suggested turn as a string

#### Scenario: No lane turns available
- **WHEN** the engine emits a LaneUpdateMessage with an empty `turns` vector
- **THEN** the Java `NavigationListener.onLaneUpdate` callback is invoked with an empty `LaneTurn[]` array

### Requirement: NavigationListener.onLaneUpdate includes turns array

The `NavigationListener.onLaneUpdate` method SHALL accept a `LaneTurn[] turns` parameter containing the per-lane turn indications. The method SHALL be a default no-op for backward compatibility.

#### Scenario: Existing listener compiles with new signature
- **WHEN** a class implements `NavigationListener` without overriding `onLaneUpdate`
- **THEN** compilation SHALL succeed and the default no-op body SHALL be used

#### Scenario: Listener receives turns array
- **WHEN** a class overrides `onLaneUpdate`
- **THEN** it SHALL receive the full `LaneTurn[]` array from the JNI bridge
