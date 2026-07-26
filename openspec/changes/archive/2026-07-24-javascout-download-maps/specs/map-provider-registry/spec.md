## ADDED Requirements

### Requirement: Register map providers

The system SHALL allow registration of map providers that define where to fetch available map listings and download map data.

- Each provider SHALL have a unique name, a base URI, and a list URI template
- The list URI template SHALL support `%1` (fromVersion), `%2` (toVersion), `%3` (locale) substitution
- Providers SHALL be loadable from JSON files at application startup
- The system SHALL expose the list of registered providers for UI selection

#### Scenario: Load providers from JSON

- **WHEN** `Settings::loadMapProviders()` is called with a path to a valid JSON file containing a provider entry
- **THEN** the provider SHALL be available via `Settings::GetMapProviders()`

#### Scenario: Load providers from multiple JSON files

- **WHEN** `Settings::loadMapProviders()` is called multiple times with different JSON files
- **THEN** all providers from all files SHALL be merged into the provider list

#### Scenario: Provider URI substitution

- **GIVEN** a provider with `listUri` = `"https://example.com/list.php?fromVersion=%1&toVersion=%2&locale=%3"`
- **WHEN** `provider.getListUri(1, 2, "en")` is called
- **THEN** the result SHALL be `"https://example.com/list.php?fromVersion=1&toVersion=2&locale=en"`

#### Scenario: Invalid JSON file

- **WHEN** `Settings::loadMapProviders()` is called with a path to an invalid or non-existent JSON file
- **THEN** the method SHALL return `false` and log a warning
- **THEN** previously loaded providers SHALL remain unchanged

### Requirement: Select active map provider

The system SHALL allow selecting which registered provider is active for fetching map listings and downloads.

- The active provider ID SHALL be persisted in `Settings`
- The default provider SHALL be the first registered provider, or none if no providers are registered

#### Scenario: Set active provider

- **GIVEN** two registered providers with names `"provider-a"` and `"provider-b"`
- **WHEN** the active provider is set to `"provider-b"`
- **THEN** `Settings::GetMapProviders()` SHALL still return both providers
- **THEN** the active provider SHALL be `"provider-b"`

#### Scenario: Persist active provider

- **WHEN** the active provider ID is changed
- **THEN** the new ID SHALL be persisted and restored on next application start

### Implementation Note

JavaScout currently hard-codes the default `karry.cz` provider in `MapDownloadController.loadProviders()` and stores only the provider name via `Config.setMapProvider()` / `Config.getMapProvider()`. Full provider registry loading from JSON is deferred.
