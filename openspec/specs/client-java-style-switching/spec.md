# client-java-style-switching Specification

## Purpose
Extends the Java client library with runtime stylesheet enumeration and switching so applications can change map styles without restarting or rebuilding.

## Requirements

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

The system SHALL provide a method that loads a named stylesheet and makes it the active style for subsequent map rendering. A stylesheet that cannot be loaded SHALL never become the active style: whenever a style was active before the attempt, it SHALL remain in effect, and the failure SHALL be surfaced to the caller together with the style that is active after the attempt. This guarantee SHALL hold on every stylesheet load path — the explicit runtime switch, the style applied when a session starts, a style-flag change, the basemap's own stylesheet, and the stylesheet refresh.

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

#### Scenario: Session start with an unloadable style

- **GIVEN** a client configured to apply style "winter-sports" when the session starts, whose stylesheet cannot be parsed
- **WHEN** the session starts and the first render is requested
- **THEN** no render SHALL fault
- **AND** the failure SHALL be reported to the caller together with the style that is active instead

#### Scenario: Style flag change with an unloadable stylesheet

- **GIVEN** a client with a valid active style
- **WHEN** a style-flag change makes the active stylesheet fail to load
- **THEN** the previously active style configuration SHALL remain in effect
- **AND** the failure SHALL be reported to the caller

#### Scenario: Basemap stylesheet fails to load

- **GIVEN** a client with a valid map style and a basemap configured with its own stylesheet
- **WHEN** the basemap stylesheet cannot be parsed while the map stylesheet loads successfully
- **THEN** map rendering SHALL continue with the map stylesheet
- **AND** the failure SHALL be reported to the caller

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

### Requirement: A database always has a usable style configuration

Every database SHALL have a usable style configuration at all times. When no stylesheet has been loaded successfully for a database yet, the client SHALL use a safe configuration that draws no content for that database. Once a stylesheet has loaded successfully, a later failed load SHALL keep that successfully loaded configuration rather than falling back to the safe one.

#### Scenario: First load of a session fails

- **GIVEN** a database whose stylesheet has never loaded successfully in this session
- **WHEN** the first load attempt fails
- **THEN** the database SHALL have the safe configuration
- **AND** a render SHALL draw no content for that database and SHALL NOT fault

#### Scenario: Later load fails after a successful one

- **GIVEN** a database with a successfully loaded style configuration
- **WHEN** a later load attempt fails
- **THEN** the database SHALL keep rendering with the successfully loaded configuration

#### Scenario: Recovery after a failed load

- **GIVEN** a database using the safe configuration after a failed load
- **WHEN** a valid stylesheet is loaded
- **THEN** the database SHALL render content with that stylesheet without recreating the client

### Requirement: Rendering never uses a style configuration from a failed load

The client SHALL NOT use a style configuration produced by a failed load, or present a database without a usable configuration, to the map painting stage. A render requested while a database has no successfully loaded stylesheet SHALL complete without a native fault. The decision SHALL be taken when a style configuration is installed, not per rendered frame, so no cost is added to rendering.

#### Scenario: Render with a failed load

- **GIVEN** a load that failed and left a database with the safe configuration
- **WHEN** a map render is requested
- **THEN** the render SHALL complete without a fault
- **AND** no content SHALL be drawn for that database

#### Scenario: Other databases still render

- **GIVEN** several databases with one of them on the safe configuration
- **WHEN** a map render is requested
- **THEN** the remaining databases SHALL still draw their content in that render

#### Scenario: No configuration is ever absent at painting time

- **GIVEN** any sequence of successful and failed stylesheet loads
- **WHEN** a map render is requested
- **THEN** no database SHALL reach the painting stage without a usable style configuration
