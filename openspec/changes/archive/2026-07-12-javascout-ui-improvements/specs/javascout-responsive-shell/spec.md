## ADDED Requirements

### Requirement: Window resize redraws map to new size
The JavaScout window SHALL redraw the map to the new canvas size whenever the window is resized.

#### Scenario: Map redraws after horizontal resize
- **WHEN** the user increases the window width
- **THEN** the map canvas width SHALL increase to fill the new space
- **AND** a new render SHALL be requested

#### Scenario: Map redraws after vertical resize
- **WHEN** the user increases the window height
- **THEN** the map canvas height SHALL increase to fill the new space
- **AND** a new render SHALL be requested

### Requirement: Resize preserves map center
When the window is resized, the geographic center of the map SHALL remain unchanged.

#### Scenario: Center stays fixed during resize
- **GIVEN** the map is centered at latitude 51.514227 and longitude 7.465279
- **WHEN** the user resizes the window
- **THEN** the next render SHALL use the same latitude and longitude for the center
- **AND** the map content at the center SHALL remain in the same relative screen position

### Requirement: Overlay controls are touch-aware
The floating search and route overlay buttons SHALL be sized and spaced for comfortable touch interaction, taking into account display DPI.

#### Scenario: Button size scales with DPI
- **GIVEN** the primary screen has a DPI of 192
- **WHEN** the overlay buttons are laid out
- **THEN** each button SHALL be at least as large as a 10mm square on the physical display
- **AND** the button size SHALL not be smaller than the desktop reference size

#### Scenario: Button spacing scales with DPI
- **GIVEN** the primary screen has a DPI of 144
- **WHEN** the overlay buttons are positioned
- **THEN** the vertical gap between the route and search buttons SHALL scale with the button size
- **AND** the gap SHALL not exceed one quarter of the button height

### Requirement: Reduced spacing between overlay buttons
The vertical spacing between the route overlay button and the search overlay button SHALL be small and consistent.

#### Scenario: Buttons appear close together
- **WHEN** the main window is at its default size
- **THEN** the bottom edge of the route button SHALL be no more than one button height above the top edge of the search button

### Requirement: Search and route panels share responsive sizing
The search panel and route panel SHALL use the same responsive sizing strategy based on the current scene width and display scale.

#### Scenario: Panels have matching max widths
- **GIVEN** the window is wider than the small-screen threshold
- **WHEN** both panels are expanded
- **THEN** the search panel and route panel SHALL have the same maximum width

#### Scenario: Panels go full-width on small screens
- **GIVEN** the window width is below the small-screen threshold
- **WHEN** either panel is expanded
- **THEN** the expanded panel SHALL stretch to fill the full width of the scene minus the edge margin

### Requirement: Dialogs are positioned correctly
Expanded search and route panels SHALL be anchored consistently in the bottom-right corner of the map panel and shall not extend outside the visible scene.

#### Scenario: Route panel stays inside window
- **WHEN** the route panel is expanded on a small window
- **THEN** the entire panel SHALL remain within the scene bounds
- **AND** the panel SHALL be anchored to the bottom-right edge

### Requirement: Search results support keyboard navigation
The search overlay SHALL allow keyboard navigation of the result list.

#### Scenario: Arrow keys move focus from search field to results
- **GIVEN** the search overlay is expanded and the search field has focus
- **WHEN** the user presses the Down or Up arrow key
- **THEN** focus SHALL move to the result list
- **AND** the first or last result SHALL be selected respectively

#### Scenario: Enter selects a search result
- **GIVEN** a result in the search result list is selected
- **WHEN** the user presses Enter
- **THEN** the selected location SHALL be used (navigate or route pick)

#### Scenario: Escape closes search overlay
- **GIVEN** the search overlay is expanded
- **WHEN** the user presses the Escape key
- **THEN** the search overlay SHALL collapse
