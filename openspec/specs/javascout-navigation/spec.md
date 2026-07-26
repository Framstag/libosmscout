# javascout-navigation

## Purpose

TBD

## Requirements

### Requirement: Navigation session can be started from a calculated route
The system SHALL allow JavaScout to start a navigation session using the route description produced by the existing route calculation API. The navigation session SHALL accept a `Vehicle` parameter that determines the vehicle type used by the navigation engine.

#### Scenario: Start navigation after route calculation
- **WHEN** a route has been successfully calculated and the user starts navigation
- **THEN** the JNI layer creates a NavigationEngine instance seeded with the same RouteDescriptionRef used to build the route geometry
- **AND** the engine is initialized with the vehicle type from the active RoutingProfile

#### Scenario: Start bicycle navigation
- **WHEN** a bicycle route has been calculated and the user starts navigation
- **THEN** the NavigationEngine is initialized with `osmscout::vehicleBicycle`
- **AND** the DataAgent filters for bicycle-routable objects

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

### Requirement: Vehicle type shown in navigation status

The JavaScout `RoutePanel` SHALL display the active vehicle type in the navigation status area when a navigation session is active.

#### Scenario: Bicycle icon in status
- **WHEN** navigation is active with `Vehicle.BICYCLE`
- **THEN** the navigation status label SHALL show a bicycle icon or "Bicycle" text
