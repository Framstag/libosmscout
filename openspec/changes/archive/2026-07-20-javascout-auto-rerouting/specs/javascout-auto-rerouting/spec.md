# javascout-auto-rerouting

## ADDED Requirements

### Requirement: Off-route condition triggers automatic reroute
The system SHALL automatically calculate a new route when the navigation engine reports an off-route condition.

#### Scenario: Vehicle leaves planned route
- **GIVEN** an active navigation session with a calculated route
- **WHEN** the engine emits `onRerouteRequest(lat, lon, bearing, destLat, destLon)`
- **THEN** the system calls `calculateRouteAsync` from the reported position to the original destination using the same routing profile
- **AND** the system shows a "Rerouting..." indicator in the route panel

#### Scenario: Reroute succeeds
- **GIVEN** a reroute calculation was triggered
- **WHEN** the route calculation completes successfully
- **THEN** the route overlay is updated with the new route geometry
- **AND** a new navigation session is started on the new route
- **AND** the "Rerouting..." indicator is cleared
- **AND** follow mode remains active

#### Scenario: Reroute fails
- **GIVEN** a reroute calculation was triggered
- **WHEN** the route calculation fails (no routable node, database error, etc.)
- **THEN** the old route overlay remains visible
- **AND** the old navigation session remains active
- **AND** a "Reroute failed" message is shown briefly in the route panel
- **AND** the "Rerouting..." indicator is cleared

### Requirement: Reroute cooldown prevents rapid re-calculation
The system SHALL ignore reroute requests for 15 seconds after a reroute attempt to prevent cascading re-calculations.

#### Scenario: Multiple off-route events
- **GIVEN** a reroute was just completed
- **WHEN** another `onRerouteRequest` fires within 15 seconds
- **THEN** the request is ignored
- **AND** no new route calculation is started

#### Scenario: Off-route persists after cooldown
- **GIVEN** a reroute was completed 15 or more seconds ago
- **WHEN** another `onRerouteRequest` fires
- **THEN** a new reroute calculation is started

### Requirement: Reroute preserves destination
The system SHALL reroute to the original destination, not a new one.

#### Scenario: Destination unchanged
- **GIVEN** a navigation session with destination D
- **WHEN** a reroute is triggered
- **THEN** the destination coordinates passed to `calculateRouteAsync` match the original destination
- **AND** the routing profile (vehicle type, avoid flags) is the same as the original route

### Requirement: Manual navigation stop cancels pending reroute
The system SHALL cancel any in-progress reroute calculation when the user manually stops navigation.

#### Scenario: User stops navigation during reroute
- **GIVEN** a reroute calculation is in progress
- **WHEN** the user stops navigation (via UI or closing the app)
- **THEN** the reroute calculation is cancelled via `cancelRoute()`
- **AND** no callbacks from the cancelled calculation are processed
