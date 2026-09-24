# Spec Delta

## ADDED Requirements

### Requirement: MSYS jobs provide the font the font-dependent tests measure against

The MSYS CI jobs that run the font-dependent tests (`MapPainterShieldTest`, `PerformanceTest-cairo-*`) SHALL make the font family those tests name resolvable to the font file the repository ships, so that the measured glyph metrics do not depend on the fonts the runner image or the MSYS2 package set happen to provide.

#### Scenario: Named family resolves to the repository font

- **WHEN** the font-dependent tests run in the MSYS job's test environment
- **THEN** the font family they name SHALL resolve to the repository's Liberation Sans font file
- **AND** the tests SHALL NOT measure a fallback font

#### Scenario: Provisioning survives a runner image or package change

- **WHEN** a runner image update or an MSYS2 package update changes the set of fonts the environment provides
- **THEN** the family the tests name SHALL still resolve to the repository's font file
- **AND** the font-dependent tests SHALL produce the same metric results as before the update

### Requirement: MSYS jobs verify the font and locale environment before running tests

Each MSYS job SHALL verify, in a step of its own before the test step, that the font family the font-dependent tests name resolves to the repository font file and that a locale is in effect, and SHALL fail that step when either check does not hold.

#### Scenario: Broken font environment fails before the tests run

- **GIVEN** an environment in which the named font family resolves to a fallback instead of the repository font file
- **WHEN** the MSYS job reaches the verification step
- **THEN** the step SHALL fail
- **AND** the test step SHALL NOT run
- **AND** the job output SHALL name the font the family resolved to

#### Scenario: Invalid locale is reported

- **GIVEN** an environment in which the locale in effect is not valid
- **WHEN** the MSYS job reaches the verification step
- **THEN** the job output SHALL name the locale in effect

#### Scenario: Verification passes on a provisioned environment

- **GIVEN** a font environment in which the named family resolves to the repository font file
- **WHEN** the MSYS job reaches the verification step
- **THEN** the step SHALL pass
- **AND** the test step SHALL run

### Requirement: MSYS test steps run with an explicit locale

Both MSYS jobs SHALL run their test steps with an explicitly set locale instead of inheriting the locale the MSYS2 environment provides, and both jobs SHALL use the same locale setting.

#### Scenario: Environment locale is valid in the cmake job

- **WHEN** the `gcc and cmake` job runs the tests in `.github/workflows/build_and test_on_msys.yml` with `-DHAVE_LOCALE` code paths active
- **THEN** the environment locale SHALL be valid
- **AND** no test SHALL log that it failed to obtain the environment locale

#### Scenario: Both MSYS jobs use the same locale

- **WHEN** inspecting the test steps of the `gcc and cmake` and `gcc and meson` jobs in `.github/workflows/build_and test_on_msys.yml`
- **THEN** both test steps SHALL declare the same locale value

### Requirement: MSYS font provisioning uses only repository content and the existing package setup

The MSYS font provisioning SHALL use the font file already in the repository and SHALL NOT require a font package that MSYS2 does not provide, a network download beyond the package installation the jobs already perform, or an additional secret.

#### Scenario: No new external dependency

- **WHEN** inspecting the MSYS job setup in `.github/workflows/build_and test_on_msys.yml`
- **THEN** the provisioned font SHALL originate from a file in the repository
- **AND** no additional secret or service SHALL be required
