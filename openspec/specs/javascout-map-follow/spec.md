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
The system SHALL allow the user to pan or zoom the map without fighting the follow mode. When follow mode is disengaged by user interaction, a re-center button SHALL appear to re-enable it.

#### Scenario: User manually pans
- **WHEN** the user drags the map while follow mode is active
- **THEN** follow mode is suspended and the map stays at the manually chosen center until follow mode is re-enabled
- **AND** a re-center button appears on the map overlay

#### Scenario: User zooms via pinch
- **WHEN** the user pinch-zooms while follow mode is active
- **THEN** follow mode is suspended
- **AND** a re-center button appears on the map overlay

#### Scenario: User zooms via buttons
- **WHEN** the user taps zoom-in or zoom-out while follow mode is active
- **THEN** follow mode is suspended
- **AND** a re-center button appears on the map overlay

#### Scenario: User zooms via scroll wheel
- **WHEN** the user scroll-zooms while follow mode is active
- **THEN** follow mode is suspended
- **AND** a re-center button appears on the map overlay

#### Scenario: User rotates the map
- **WHEN** the user two-finger rotates while follow mode is active
- **THEN** follow mode is suspended
- **AND** a re-center button appears on the map overlay

#### Scenario: User re-enables follow mode
- **WHEN** the user taps the re-center button
- **THEN** follow mode is re-enabled
- **AND** the map re-centers on the current GPS position
- **AND** the re-center button is hidden

### Requirement: Follow state is visible in the UI
The system SHALL indicate whether follow mode is currently active and provide a visible way to re-enable it when inactive.

#### Scenario: Toggle follow
- **WHEN** the user toggles the follow button in the settings sheet
- **THEN** the button reflects the active/inactive state

#### Scenario: Re-center button visible when disengaged
- **WHEN** follow mode was disengaged by user interaction
- **THEN** a re-center button is visible on the map overlay
