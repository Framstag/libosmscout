## ADDED Requirements

### Requirement: JavaScout exposes configurable icon directory
The system SHALL allow JavaScout to be launched with an `--icon-dir` command-line argument that specifies the directory containing icons for the standard stylesheet.

#### Scenario: Default icon directory used
- **WHEN** JavaScout is launched without an explicit `--icon-dir` (e.g. via `javascout.sh`)
- **THEN** the system SHALL use `libosmscout/data/icons/14x14/standard/` as the icon directory

#### Scenario: Legacy SVG default is migrated
- **WHEN** `config.properties` contains the old default `icon.directory=libosmscout/data/icons/svg/standard`
- **THEN** `Config.getIconDirectory()` SHALL return `libosmscout/data/icons/14x14/standard`

#### Scenario: Custom icon directory via command line
- **WHEN** JavaScout is launched with `--icon-dir /path/to/icons`
- **THEN** the system SHALL use `/path/to/icons` as the icon directory

### Requirement: Icon directory is persisted
The system SHALL persist the configured icon directory in `config.properties` under the key `icon.directory`.

#### Scenario: Default is persisted on first launch
- **WHEN** JavaScout is launched without `--icon-dir` and `config.properties` does not contain `icon.directory`
- **THEN** the system SHALL save the default directory to `config.properties`

#### Scenario: Custom value is persisted
- **WHEN** JavaScout is launched with `--icon-dir /custom/icons`
- **THEN** the system SHALL save `/custom/icons` to `icon.directory` in `config.properties`

#### Scenario: Launcher logs native library path
- **WHEN** `javascout.sh` runs
- **THEN** it SHALL print the chosen native library path to stderr before launching Java

#### Scenario: Saved value is loaded on restart
- **WHEN** JavaScout is restarted and `config.properties` contains `icon.directory=/saved/icons`
- **THEN** the system SHALL load `/saved/icons` as the icon directory

### Requirement: Config exposes icon directory key
`Config` SHALL provide read and write access for the `icon.directory` property.

#### Scenario: Read icon directory
- **WHEN** `Config.getIconDirectory()` is called
- **THEN** it SHALL return the value stored under `icon.directory`, or the default if absent

#### Scenario: Write icon directory
- **WHEN** `Config.setIconDirectory(path)` is called
- **THEN** it SHALL store `path` under `icon.directory` and make it available to `Config.save()`

### Requirement: Icon directory is forwarded to the native client
The configured icon directory SHALL be passed to `OSMScoutClientBuilder.withIconDirectory()` during native client initialisation.

#### Scenario: Native client configured with icon directory
- **WHEN** `MainController` builds the `OSMScoutClient`
- **THEN** it SHALL call `OSMScoutClientBuilder.withIconDirectory(configuredDirectory)`

### Requirement: Hard-coded icon path removed
The system SHALL NOT contain a hard-coded icon directory path in `MainController`.

#### Scenario: MainController uses configured path
- **WHEN** `MainController` initialises the native client
- **THEN** it SHALL use only the directory supplied by `JavaScoutApp` from the `Config`
