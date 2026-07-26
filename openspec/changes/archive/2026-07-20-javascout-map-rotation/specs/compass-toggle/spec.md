## ADDED Requirements

### Requirement: Compass button toggles rotation mode

A compass icon button SHALL be displayed on the map overlay. Tapping it SHALL toggle between NORTH_UP and DRIVING_DIRECTION_UP rotation modes.

#### Scenario: Toggle from north-up to driving-direction-up
- **WHEN** the compass button is tapped while rotation mode is NORTH_UP
- **THEN** the rotation mode SHALL change to DRIVING_DIRECTION_UP
- **AND** the compass icon SHALL update to indicate the new mode

#### Scenario: Toggle from driving-direction-up to north-up
- **WHEN** the compass button is tapped while rotation mode is DRIVING_DIRECTION_UP
- **THEN** the rotation mode SHALL change to NORTH_UP
- **AND** the map SHALL immediately rotate back to north-up (angle=0)

### Requirement: Compass icon rotates to show true north

The compass icon SHALL visually rotate to indicate the direction of true north, even when the map itself is rotated. The icon SHALL use a red/white color scheme (red points north).

#### Scenario: Compass points north when map is north-up
- **WHEN** the map is in NORTH_UP mode (angle=0)
- **THEN** the compass icon SHALL point straight up

#### Scenario: Compass rotates when map rotates
- **WHEN** the map is rotated 90° clockwise in DRIVING_DIRECTION_UP mode
- **THEN** the compass icon SHALL be rotated 90° counter-clockwise relative to the map, so the red pointer still indicates true north

### Requirement: Compass button visual states

The compass button SHALL have two visual states:
- NORTH_UP mode: compass icon with north pointer up, semi-transparent background
- DRIVING_DIRECTION_UP mode: compass icon rotated to show north, highlighted/colored background to indicate active mode

#### Scenario: Visual state reflects current mode
- **WHEN** rotation mode is NORTH_UP
- **THEN** the compass button SHALL show the default (unhighlighted) style
- **WHEN** rotation mode is DRIVING_DIRECTION_UP
- **THEN** the compass button SHALL show the active (highlighted) style

### Requirement: Compass button positioned on map overlay

The compass button SHALL be positioned in the bottom-right corner of the map, above the follow-mode button, with consistent spacing matching other overlay buttons.

#### Scenario: Compass button layout
- **WHEN** the map is displayed
- **THEN** the compass button SHALL be visible in the bottom-right corner
- **AND** it SHALL be positioned above the follow-mode button
- **AND** it SHALL use the same size and spacing as other overlay buttons
