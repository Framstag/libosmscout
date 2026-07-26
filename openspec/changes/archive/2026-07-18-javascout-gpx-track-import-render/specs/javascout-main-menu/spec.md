# javascout-main-menu

## Purpose

Provide a top-left menu in JavaScout that groups map-related actions such as favorites management and GPX track import.

## ADDED Requirements

### Requirement: Top-left menu button replaces standalone favorites button

The system SHALL replace the standalone favorites star button with a menu button in the top-left corner of the map.

#### Scenario: Menu button is visible

- **WHEN** JavaScout starts and the map is visible
- **THEN** a menu button SHALL appear in the top-left corner of the map panel
- **AND** the standalone favorites button SHALL NOT be visible

#### Scenario: Menu opens on click

- **GIVEN** the menu button is visible
- **WHEN** the user clicks the menu button
- **THEN** a menu SHALL open showing at least "Favorites" and "Import GPX Track…" items

### Requirement: Favorites action opens the favorites dialog

The system SHALL provide a "Favorites" menu item that opens the existing favorite locations management dialog.

#### Scenario: Favorites menu item opens dialog

- **GIVEN** the menu is open
- **WHEN** the user selects "Favorites"
- **THEN** the `FavLocationDialog` SHALL open
- **AND** the menu SHALL close

### Requirement: Import GPX Track menu item triggers file chooser

The system SHALL provide an "Import GPX Track…" menu item that triggers the GPX file chooser.

#### Scenario: Import menu item opens file chooser

- **GIVEN** the menu is open
- **WHEN** the user selects "Import GPX Track…"
- **THEN** a file chooser filtered to `.gpx` files SHALL open
- **AND** the menu SHALL close

### Requirement: Menu is keyboard dismissible

The system SHALL allow the user to dismiss the open menu with the Escape key or by clicking outside the menu.

#### Scenario: Escape closes menu

- **GIVEN** the menu is open
- **WHEN** the user presses Escape
- **THEN** the menu SHALL close without invoking any action

#### Scenario: Click outside closes menu

- **GIVEN** the menu is open
- **WHEN** the user clicks outside the menu bounds
- **THEN** the menu SHALL close without invoking any action
