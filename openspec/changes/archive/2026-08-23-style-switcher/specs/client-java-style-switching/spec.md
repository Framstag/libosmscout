# client-java-style-switching

## Purpose

Extends the Java client library with runtime stylesheet enumeration and switching so applications can change map styles without restarting or rebuilding.

## ADDED Requirements

### Requirement: Client enumerates available styles

The system SHALL provide a client API that returns the names of all available map styles, derived from the top-level `*.oss` files in the configured stylesheets directory.

#### Scenario: Style names from stylesheet files

- **GIVEN** the stylesheets directory contains `standard.oss`, `cycle.oss`, and `railways.oss`
- **WHEN** the client queries the available styles
- **THEN** the result SHALL contain "standard", "cycle", and "railways"

#### Scenario: Stylesheet directory not configured

- **GIVEN** no stylesheets directory has been configured for the client
- **WHEN** the client queries the available styles
- **THEN** the query SHALL return the styles found in the default stylesheets directory

#### Scenario: Empty stylesheet directory

- **GIVEN** the stylesheets directory contains no `*.oss` files
- **WHEN** the client queries the available styles
- **THEN** the query SHALL return an empty list

### Requirement: Client switches active style at runtime

The system SHALL provide a method that loads a named stylesheet and makes it the active style for subsequent map rendering.

#### Scenario: Switching to a valid style succeeds

- **GIVEN** a client with style "standard" active
- **WHEN** the client switches to style "cycle"
- **THEN** the switch SHALL succeed
- **AND** subsequent map renders SHALL use style "cycle"

#### Scenario: Querying the active style

- **GIVEN** a client whose active style is "cycle"
- **WHEN** the client queries the active style
- **THEN** the query SHALL return "cycle"

#### Scenario: Switching to an unknown style fails

- **GIVEN** a client with style "standard" active
- **WHEN** the client attempts to switch to a style that does not exist in the stylesheets directory
- **THEN** the switch SHALL fail
- **AND** the active style SHALL remain "standard"

#### Scenario: Switching to an unloadable stylesheet fails

- **GIVEN** a client with style "standard" active
- **WHEN** the client switches to a style whose stylesheet file cannot be parsed
- **THEN** the switch SHALL fail
- **AND** the active style SHALL remain "standard"
- **AND** the failure SHALL be surfaced to the caller

#### Scenario: Style flags survive a switch

- **GIVEN** a client with style flags such as "daylight" enabled
- **WHEN** the client switches to a different style
- **THEN** the enabled flags SHALL be applied to the newly selected style

### Requirement: Client signals redraw need after switch

The system SHALL notify the application after a successful style switch that the map needs to be redrawn with the new style.

#### Scenario: Redraw notification after switch

- **GIVEN** a successful switch to style "cycle"
- **WHEN** the switch completes
- **THEN** the application SHALL be notified that a redraw is required

#### Scenario: No notification on failed switch

- **GIVEN** a switch attempt to a style that fails to load
- **WHEN** the switch attempt completes
- **THEN** the application SHALL NOT be notified of a redraw requirement
