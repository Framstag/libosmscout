# javascout-navigation

## ADDED Requirements

### Requirement: Navigation session can be started from a calculated route
The system SHALL allow JavaScout to start a navigation session using the route description produced by the existing route calculation API.

#### Scenario: Start navigation after route calculation
- **WHEN** a route has been successfully calculated and the user starts navigation
- **THEN** the JNI layer creates a NavigationEngine instance seeded with the same RouteDescriptionRef used to build the route geometry

### Requirement: JavaScout can feed GPS updates to the navigation engine
The system SHALL accept periodic GPS updates from Java code and pass them to the C++ NavigationEngine as GPSUpdateMessage and TimeTickMessage.

#### Scenario: Feed simulated GPS fix
- **WHEN** a Java caller invokes `navigationController.processLocation(lat, lon, speed, accuracy, timestamp)`
- **THEN** the C++ engine processes the update and emits the resulting NavigationMessage list back to Java

### Requirement: Navigation engine reports snapped vehicle position
The system SHALL report the NavigationEngine's estimated vehicle position back to Java after each GPS update.

#### Scenario: Receive position estimate
- **WHEN** the engine processes a GPS update
- **THEN** the Java `NavigationListener.onPositionEstimate(state, lat, lon, bearing)` callback is invoked with the snapped position, state, and optional bearing

### Requirement: Off-route condition is reported
The system SHALL report when the estimated position state becomes `OffRoute` so the application can trigger rerouting.

#### Scenario: Vehicle leaves planned route
- **WHEN** the snapped position state transitions to `OffRoute`
- **THEN** the Java `NavigationListener.onRerouteRequest(lat, lon, bearing, destLat, destLon)` callback is invoked

### Requirement: Target reached event is reported
The system SHALL report when the vehicle approaches or reaches the destination.

#### Scenario: Arrival at destination
- **WHEN** the engine emits a TargetReachedMessage
- **THEN** the Java `NavigationListener.onTargetReached(bearing, distance)` callback is invoked

### Requirement: Speed and lane information are reported
The system SHALL forward current speed, max allowed speed, and lane information from the engine to Java listeners.

#### Scenario: Speed limit changes
- **WHEN** the engine emits a CurrentSpeedMessage, MaxAllowedSpeedMessage, or LaneUpdateMessage
- **THEN** the corresponding Java `NavigationListener` callback is invoked

### Requirement: Voice instruction samples are reported
The system SHALL forward voice-instruction sample identifiers to Java so the UI can play them.

#### Scenario: Voice instruction emitted
- **WHEN** the engine emits a VoiceInstructionMessage
- **THEN** the Java `NavigationListener.onVoiceInstruction(samples)` callback is invoked
