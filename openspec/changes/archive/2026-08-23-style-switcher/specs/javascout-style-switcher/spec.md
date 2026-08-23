# javascout-style-switcher

## Purpose

Lets JavaScout users change the map's visual style at runtime by selecting one of the available stylesheets, with the map redrawn immediately.

## ADDED Requirements

### Requirement: Style switcher entry point in main menu

The system SHALL provide a "Switch Style…" menu item in the JavaScout main menu that opens the style chooser dialog.

#### Scenario: Menu item is available

- **WHEN** the JavaScout main menu is open
- **THEN** a "Switch Style…" menu item SHALL be listed

#### Scenario: Menu item opens style chooser

- **WHEN** the user selects "Switch Style…"
- **THEN** the style chooser dialog SHALL open
- **AND** the main menu SHALL close

### Requirement: Style candidates from stylesheet files

The system SHALL derive the list of selectable styles from the top-level `*.oss` files in the stylesheets directory, naming each style after the file name without the `.oss` extension.

#### Scenario: Styles listed from stylesheet files

- **GIVEN** the stylesheets directory contains `standard.oss`, `cycle.oss`, and `railways.oss`
- **WHEN** the style chooser dialog is open
- **THEN** the dialog SHALL list the styles "standard", "cycle", and "railways"

#### Scenario: Non-stylesheet files are ignored

- **GIVEN** the stylesheets directory contains a file named `notes.txt` alongside `*.oss` files
- **WHEN** the style chooser dialog is open
- **THEN** the dialog SHALL NOT list "notes"

#### Scenario: No stylesheet files present

- **GIVEN** the stylesheets directory contains no `*.oss` files
- **WHEN** the style chooser dialog is open
- **THEN** the dialog SHALL indicate that no styles are available
- **AND** the dialog SHALL NOT allow confirming a style switch

### Requirement: Style selection applies immediately

The system SHALL redraw the map with the newly selected style as soon as the user confirms the selection in the chooser.

#### Scenario: Map redraws with new style

- **GIVEN** the map is displayed using style "standard"
- **WHEN** the user selects style "cycle" and confirms
- **THEN** the map SHALL be redrawn using style "cycle"
- **AND** the rendering SHALL reflect the new style's rules

#### Scenario: Map view is preserved

- **GIVEN** the map is centered on a location at a given zoom level
- **WHEN** the user switches style
- **THEN** the map center and zoom level SHALL remain unchanged

#### Scenario: Current style preselected

- **GIVEN** the map is currently rendered with style "standard"
- **WHEN** the style chooser dialog opens
- **THEN** "standard" SHALL be preselected in the chooser

#### Scenario: Switching to the current style has no effect

- **GIVEN** the map is rendered with style "standard"
- **WHEN** the user confirms "standard" in the chooser
- **THEN** the map SHALL remain rendered with style "standard"

#### Scenario: Dialog dismissed without change

- **GIVEN** the style chooser dialog is open
- **WHEN** the user cancels the dialog
- **THEN** the active style SHALL remain unchanged

### Requirement: Style switcher reports failures

The system SHALL not change the active style when the chosen stylesheet cannot be loaded.

#### Scenario: Unloadable stylesheet keeps previous style

- **GIVEN** the map is rendered with style "standard"
- **WHEN** the user selects a style whose stylesheet fails to load and confirms
- **THEN** the map SHALL remain rendered with style "standard"
- **AND** the failure SHALL be reported to the user
