# javascout-map-follow

## Purpose

TBD

## Requirements

### Requirement: Map can auto-center on current navigation position
The system SHALL provide a follow mode in which the map view is re-centered on the latest reported navigation position.

#### Scenario: Follow mode enabled
- **WHEN** follow mode is enabled and a new position estimate arrives
- **THEN** the map renderer requests a render centered on the estimated position at the current magnification

### Requirement: User can disable follow mode
The system SHALL allow the user to pan or zoom the map without fighting the follow mode.

#### Scenario: User manually pans
- **WHEN** the user drags the map while follow mode is active
- **THEN** follow mode is suspended and the map stays at the manually chosen center until follow mode is re-enabled

### Requirement: Follow state is visible in the UI
The system SHALL indicate whether follow mode is currently active.

#### Scenario: Toggle follow
- **WHEN** the user toggles the follow button
- **THEN** the button reflects the active/inactive state
