## MODIFIED Requirements

### Requirement: Application window
JavaScout SHALL display a JavaFX application window (Stage) with a title "JavaScout".

#### Scenario: Window appears on launch
- **WHEN** user launches JavaScout
- **THEN** a JavaFX window with title "JavaScout" SHALL appear

#### Scenario: Window has minimum size
- **WHEN** the window appears
- **THEN** it SHALL have a minimum width of 800px and minimum height of 600px

#### Scenario: No menu bar
- **WHEN** the window appears
- **THEN** there SHALL be no menu bar

### Requirement: Map panel placeholder
The window SHALL contain a central content area reserved for map rendering.

#### Scenario: Empty map panel is visible
- **WHEN** the window appears
- **AND** no database is loaded
- **THEN** the central area SHALL show a placeholder message "No map loaded"

#### Scenario: Map panel fills available space
- **WHEN** the window is resized
- **THEN** the central map panel SHALL resize to fill the available space between top and status bar
- **AND** the map SHALL be redrawn to the new size
- **AND** the center of the map SHALL remain the same

## ADDED Requirements

### Requirement: Application passes display scale to overlays
The JavaScout application shell SHALL compute a UI scale factor from the primary screen DPI and make it available to overlay controls.

#### Scenario: DPI is detected on startup
- **WHEN** the application starts
- **THEN** the shell SHALL read the primary screen DPI
- **AND** compute a UI scale factor of at least 1.0
- **AND** pass the scale to search and route overlays

### Requirement: Keyboard shortcuts open search and route dialogs
The JavaScout shell SHALL provide keyboard shortcuts to open the search overlay and the route panel.

#### Scenario: Ctrl+F opens search
- **WHEN** the user presses Ctrl+F
- **THEN** the search overlay SHALL open

#### Scenario: Ctrl+R toggles route panel
- **WHEN** the user presses Ctrl+R
- **THEN** the route panel SHALL open if closed
- **AND** the route panel SHALL close if open

### Requirement: Zoom overlay buttons
The JavaScout shell SHALL provide on-screen zoom in and zoom out buttons.

#### Scenario: Zoom in button increases magnification
- **WHEN** the user clicks the zoom in button
- **THEN** the map magnification SHALL increase by one level
- **AND** the map SHALL re-render with the new magnification

#### Scenario: Zoom out button decreases magnification
- **WHEN** the user clicks the zoom out button
- **THEN** the map magnification SHALL decrease by one level
- **AND** the map SHALL re-render with the new magnification

#### Scenario: Keyboard shortcuts only fire when map has focus
- **GIVEN** focus is in an overlay text field or result list
- **WHEN** the user presses Ctrl+F or Ctrl+R
- **THEN** the shortcut SHALL NOT trigger
- **AND** the native text-field behavior (if any) SHALL remain

#### Scenario: Clicking map gives it focus
- **WHEN** the user clicks on the map canvas
- **THEN** the map canvas SHALL gain focus
- **AND** subsequent Ctrl+F / Ctrl+R shortcuts SHALL trigger
