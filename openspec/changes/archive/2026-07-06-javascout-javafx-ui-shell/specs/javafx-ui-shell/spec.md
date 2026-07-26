## ADDED Requirements

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

### Requirement: Config file
JavaScout SHALL read configuration from `~/.config/javascout/config.properties` (Linux), `~/Library/Application Support/JavaScout/config.properties` (macOS), or `%APPDATA%\JavaScout\config.properties` (Windows).

#### Scenario: Config file exists with maps.directory
- **WHEN** the config file exists
- **AND** contains `maps.directory=/home/user/maps`
- **THEN** JavaScout SHALL use `/home/user/maps` as the database directory

#### Scenario: Config file does not exist
- **WHEN** no config file exists
- **AND** no CLI argument is provided
- **THEN** the window SHALL appear with empty state
- **AND** the status bar SHALL show "No maps directory configured"

#### Scenario: CLI argument overrides config
- **WHEN** user runs `javascout.sh /custom/path`
- **THEN** JavaScout SHALL use `/custom/path` as the database directory
- **AND** SHALL ignore the config file value

#### Scenario: CLI argument is saved to config
- **WHEN** user runs `javascout.sh /custom/path`
- **THEN** the config file SHALL be created/updated with `maps.directory=/custom/path`

### Requirement: Database directory scanning
JavaScout SHALL scan the configured database directory and load all `.osmscout` databases found within.

#### Scenario: Directory contains one database
- **WHEN** the database directory contains a valid `.osmscout` database
- **THEN** JavaScout SHALL call `OSMScoutClient.openDatabase()` with the directory path
- **AND** the status bar SHALL show the database directory path

#### Scenario: Directory contains multiple databases
- **WHEN** the database directory contains multiple `.osmscout` databases
- **THEN** JavaScout SHALL load all of them
- **AND** the status bar SHALL show the directory path

#### Scenario: Directory is invalid
- **WHEN** the configured directory does not contain valid `.osmscout` data
- **THEN** the window SHALL show empty state
- **AND** the status bar SHALL show "No databases found"

### Requirement: Map panel placeholder
The window SHALL contain a central content area reserved for map rendering.

#### Scenario: Empty map panel is visible
- **WHEN** the window appears
- **AND** no database is loaded
- **THEN** the central area SHALL show a placeholder message "No map loaded"

#### Scenario: Map panel fills available space
- **WHEN** the window is resized
- **THEN** the central map panel SHALL resize to fill the available space between top and status bar

### Requirement: Status bar
The window SHALL contain a status bar at the bottom showing database directory and coordinates.

#### Scenario: Status bar shows database directory
- **WHEN** a database directory is loaded
- **THEN** the status bar SHALL display the directory path on the left side

#### Scenario: Status bar shows coordinate placeholder
- **WHEN** no database is loaded
- **THEN** the status bar SHALL show "Lat: -- Lon: --" on the right side

### Requirement: Maven build with JavaFX
The Maven build SHALL include JavaFX dependencies and support running via `mvn javafx:run`.

#### Scenario: Build succeeds with JavaFX
- **WHEN** user runs `mvn package` in JavaScout/
- **THEN** the build SHALL succeed
- **AND** produce an executable JAR

#### Scenario: Run via Maven plugin
- **WHEN** user runs `mvn javafx:run` in JavaScout/
- **THEN** the JavaFX application SHALL launch
