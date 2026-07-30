## ADDED Requirements

### Requirement: TODO.md documents missing skia backend features

The system SHALL create a `TODO.md` file at `libosmscout-map-skia/TODO.md` that documents all features still missing from the Skia map backend compared to the Cairo and Qt backends.

#### Scenario: TODO.md exists at top level

- **WHEN** the change is complete
- **THEN** a file `libosmscout-map-skia/TODO.md` SHALL exist

#### Scenario: TODO.md lists icon rendering

- **WHEN** the TODO.md is read
- **THEN** it SHALL mention that `StyleSheetChanged` cleanup for icon and pattern caches is not yet implemented

#### Scenario: TODO.md lists pattern support

- **WHEN** the TODO.md is read
- **THEN** it SHALL mention that SVG icon loading is not yet supported (matching Cairo's known limitation)

#### Scenario: TODO.md lists StyleSheetChanged

- **WHEN** the TODO.md is read
- **THEN** it SHALL mention that `StyleSheetChanged` does not clear icon/pattern caches

#### Scenario: TODO.md lists any other known gaps

- **WHEN** the TODO.md is read
- **THEN** it SHALL include any other feature gaps identified between the Skia backend and the Cairo/Qt backends
