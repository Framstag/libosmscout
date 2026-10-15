# Spec Delta

## ADDED Requirements

### Requirement: Required build dependencies are provided by every entry point that builds the component

Every CI entry point that configures or builds a component SHALL provide that component's required
build dependencies before configuring it. A configuration that deliberately does not build a
component SHALL NOT be required to provide that component's dependencies, and SHALL be documented as
exempt rather than left out. When a required build dependency cannot be satisfied, the run SHALL fail
before any release artifact is published and SHALL report which dependency is missing.

#### Scenario: Release workflows provide the import library's JSON dependency

- **GIVEN** the import library declares a required JSON implementation
- **WHEN** the dependency installation step of a release workflow is inspected
- **THEN** the JSON implementation SHALL be installed by that workflow before the project is
  configured

#### Scenario: Every release entry point is covered

- **GIVEN** more than one workflow can create a release
- **WHEN** each release workflow's dependency installation step is inspected
- **THEN** every release workflow that configures the project SHALL install the required JSON
  implementation, and no release workflow SHALL rely on another workflow's installation step

#### Scenario: Build and test workflows already provide it

- **GIVEN** a workflow that builds the import library for pull-request validation
- **WHEN** its dependency installation step is inspected
- **THEN** the required JSON implementation SHALL be installed by that workflow

#### Scenario: A configuration that does not build the import library is exempt

- **GIVEN** a build configuration that disables the import library
- **WHEN** that configuration is configured without the JSON implementation present
- **THEN** configuration SHALL succeed without the JSON implementation being required

#### Scenario: An unsatisfied required dependency stops the release

- **GIVEN** a release workflow whose environment lacks a dependency the configured project requires
- **WHEN** the workflow runs
- **THEN** the run SHALL end in failure before any release artifact is published
- **AND** the run SHALL report the name of the missing dependency
