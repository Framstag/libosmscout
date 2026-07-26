## Purpose

Handle keyboard and mouse interaction for the map view. Supports pan (keyboard arrows, mouse drag) and zoom (keyboard +/-, scroll wheel centered on cursor).

## Requirements

### Requirement: Keyboard map navigation
The system SHALL support keyboard-based map pan and zoom.

#### Scenario: Arrow key pan
- **WHEN** user presses Up arrow key
- **THEN** map view pans north by ~10% of viewport height
- **WHEN** user presses Down arrow key
- **THEN** map view pans south by ~10% of viewport height
- **WHEN** user presses Left arrow key
- **THEN** map view pans west by ~10% of viewport width
- **WHEN** user presses Right arrow key
- **THEN** map view pans east by ~10% of viewport width

#### Scenario: Page key pan
- **WHEN** user presses Page Up
- **THEN** map view pans north by ~50% of viewport height
- **WHEN** user presses Page Down
- **THEN** map view pans south by ~50% of viewport height

#### Scenario: Keyboard zoom
- **WHEN** user presses +
- **THEN** magnification increases by one level
- **WHEN** user presses -
- **THEN** magnification decreases by one level

### Requirement: Mouse map navigation
The system SHALL support mouse-based map pan and zoom.

#### Scenario: Mouse drag pan
- **WHEN** user presses mouse button on map and drags
- **THEN** map view follows mouse movement (drag-to-pan)
- **WHEN** user releases mouse button
- **THEN** panning stops and map re-renders at new position

#### Scenario: Scroll wheel zoom
- **WHEN** user scrolls mouse wheel up
- **THEN** magnification increases by one level centered on cursor position
- **WHEN** user scrolls mouse wheel down
- **THEN** magnification decreases by one level centered on cursor position

### Requirement: Focus handling
The system SHALL respond to keyboard input only when the map panel has focus.

#### Scenario: Keyboard focus
- **WHEN** map panel has focus
- **THEN** keyboard pan and zoom keys are active
- **WHEN** map panel loses focus
- **THEN** keyboard pan and zoom keys are ignored
