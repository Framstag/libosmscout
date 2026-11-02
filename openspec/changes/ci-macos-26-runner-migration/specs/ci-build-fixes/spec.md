# Spec Delta

## ADDED Requirements

### Requirement: A CI job is scheduled on a runner image that is still supported

Every job SHALL name a runner image label that the runner-image project still supports. A label that
project marks deprecated SHALL NOT be named, and a label it has retired SHALL NOT be named. When a
named macOS label reaches its retirement, every macOS job SHALL move in the same change, so the
repository never has a job that cannot be scheduled. A macOS job SHALL name a concrete image version
rather than the `-latest` alias, so that an image move is a reviewed change.

#### Scenario: No job names a deprecated or retired image label

- **GIVEN** the runner-image project publishes the support state of every image label
- **WHEN** the `runs-on` value of every job under `.github/workflows/` is inspected
- **THEN** no job SHALL name a label that project marks deprecated
- **AND** no job SHALL name a label that project has retired

#### Scenario: The macOS jobs move with the image they name

- **GIVEN** a macOS runner image label is scheduled for retirement
- **WHEN** the change that migrates the repository is inspected
- **THEN** every job that builds on macOS SHALL name a label the runner-image project still supports
- **AND** the migration SHALL cover the jobs of every workflow that named the retired label

#### Scenario: A macOS job names an image version, not the latest alias

- **WHEN** the `runs-on` value of a macOS job is inspected
- **THEN** it SHALL name a concrete image version such as `macos-26`
- **AND** it SHALL NOT use the `macos-latest` alias

### Requirement: An Apple platform build check configures a deployment target its toolchain accepts

A job that builds for an Apple platform SHALL pass a deployment target that the Xcode of its runner
image supports, and SHALL state the reason where the value sits when that value is the lowest the
toolchain accepts. When the deployment target of such a job is higher than the deployment target the
shipped Apple projects declare, that difference SHALL be recorded as an open item of the project.

#### Scenario: The iOS check configures the floor of its toolchain

- **GIVEN** the iOS job builds with the Xcode of its runner image
- **WHEN** its configure step is inspected
- **THEN** the deployment target it passes SHALL be at least the lowest deployment target that Xcode
  supports
- **AND** the workflow SHALL state that the value is that floor

#### Scenario: A check above the shipped deployment target is recorded

- **GIVEN** an Apple platform job passes a deployment target above the one the shipped Apple projects
  declare
- **WHEN** the repository's open items are inspected
- **THEN** they SHALL name the job, the value it passes and the value the projects declare
