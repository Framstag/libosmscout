# Spec Delta

## REMOVED Requirements

### Requirement: CI workflows install libpng

**Reason**: The requirement named the libpng package of one MSYS2 environment, `mingw-w64-x86_64-libpng`. The MSYS jobs no longer select that environment, and the requirement as written would send a reader to a package the jobs do not install.

**Migration**: Replaced by "CI workflows install the libpng package of their environment", which keeps the Ubuntu (`libpng-dev`) and macOS (`libpng`) entries and states the MSYS entry as the libpng package of the environment the job selects.

## ADDED Requirements

### Requirement: CI workflows install the libpng package of their environment

Every CI workflow that builds demos SHALL install libpng, named the way its package manager names it — `libpng-dev` on Ubuntu, `libpng` on macOS, and the libpng package of the MSYS2 environment the job selects (currently `mingw-w64-ucrt-x86_64-libpng`) — so that `PNG::PNG` is always available.

#### Scenario: Ubuntu CI apt-get includes libpng-dev

- **WHEN** inspecting the `apt-get install` commands in `.github/workflows/build_and test_on_ubuntu_26_04.yml`
- **THEN** `libpng-dev` SHALL appear in every package list for jobs that build demos

#### Scenario: macOS CI brew includes libpng

- **WHEN** inspecting the `brew install` command in `.github/workflows/build_and test_on_osx.yml`
- **THEN** `libpng` SHALL appear in the package list

#### Scenario: MSYS CI includes the libpng package of its environment

- **WHEN** inspecting the `setup-msys2` install list in `.github/workflows/build_and test_on_msys.yml`
- **THEN** the libpng package of the environment the job selects SHALL appear in the package list
- **AND** its entry SHALL carry that environment's prefix

### Requirement: An MSYS2 CI job selects an environment that carries every package it installs

Every MSYS2 job SHALL select its environment explicitly rather than relying on the action's default,
and every package the job installs SHALL be published for the environment it selects. When MSYS2
stops publishing a package for that environment, the job SHALL move to an environment that still
carries the package set the repository builds with, instead of dropping the package from the install
list.

#### Scenario: The install list resolves in the selected environment

- **GIVEN** an MSYS2 job with a setup step that selects an environment
- **WHEN** the packages of its install list are looked up in that environment's package repository
- **THEN** every package SHALL be available there
- **AND** the install step SHALL NOT fail with `error: target not found`

#### Scenario: A package removed from an environment moves the job, not the package list

- **GIVEN** MSYS2 stops publishing a package for the environment a job selects
- **AND** the package is one the project's build uses
- **WHEN** the change that responds to the removal is inspected
- **THEN** the job SHALL select an environment that carries the package
- **AND** the package SHALL stay in the install list

#### Scenario: A package the project does not use may leave the list

- **GIVEN** MSYS2 stops publishing a package for the environment a job selects
- **AND** nothing in the repository uses that package
- **WHEN** the change that responds to the removal is inspected
- **THEN** the package SHALL be absent from the install list
- **AND** the reason SHALL be stated next to the list

#### Scenario: Every entry carries the prefix of the selected environment

- **WHEN** the install list of a job that builds demos is inspected
- **THEN** every entry SHALL carry the prefix of the environment that job selects
- **AND** no entry SHALL name the prefix of an environment the job does not select

### Requirement: A deprecated MSYS2 environment is left, not acknowledged

An MSYS2 job SHALL NOT run in an environment MSYS2 marks deprecated, and SHALL NOT acknowledge that
deprecation with `suppress-deprecation-warnings`. The environment a job runs in SHALL be stated in
the setup step that selects it, and the steps that depend on the environment's installation prefix
SHALL read it from the environment rather than naming it.

#### Scenario: No MSYS2 job suppresses a deprecation warning

- **WHEN** the setup steps of the MSYS2 jobs under `.github/workflows/` are inspected
- **THEN** none of them SHALL pass `suppress-deprecation-warnings`
- **AND** each SHALL pass a `msystem` value that MSYS2 does not mark deprecated

#### Scenario: The steps that need the prefix read it from the environment

- **GIVEN** a job that registers a font configuration, sets the DirectX SDK directory or extends the
  test `PATH` with the environment's binary directory
- **WHEN** those steps are inspected
- **THEN** they SHALL name the installation prefix through the environment's own variables
- **AND** they SHALL NOT hardcode the path of a particular environment

#### Scenario: The environment is stated where it is selected

- **WHEN** a reader inspects an MSYS2 job of `.github/workflows/build_and test_on_msys.yml`
- **THEN** the setup step SHALL name the environment it selects
- **AND** the setup step or the comment above it SHALL state why that environment is used