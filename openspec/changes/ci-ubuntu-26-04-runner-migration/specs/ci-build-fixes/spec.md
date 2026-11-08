# Spec Delta

## ADDED Requirements

### Requirement: A Linux CI job is scheduled on the Ubuntu release its workflow names

Every job that builds on Linux SHALL name a runner image label that the runner-image project ships and
still supports. The label SHALL be the concrete release, not the moving `-latest` alias, so that an
image move is a reviewed change. A workflow file that carries the release in its name SHALL name the
release its jobs are scheduled on.

#### Scenario: The Linux jobs name the current LTS image

- **WHEN** the `runs-on` value of every job that builds on Linux under `.github/workflows/` is inspected
- **THEN** each of them SHALL name a concrete Ubuntu label such as `ubuntu-26.04`
- **AND** none of them SHALL name the `ubuntu-latest` alias
- **AND** none of them SHALL name a label the runner-image project has marked deprecated or retired

#### Scenario: A workflow file name states the release it runs on

- **GIVEN** a workflow file whose name contains a Ubuntu release
- **WHEN** its `runs-on` values are inspected
- **THEN** the release in the file name SHALL equal the release of the label
- **AND** the `name:` key of the workflow SHALL state the same release

#### Scenario: Documentation names the workflows that exist

- **WHEN** the build-status table of `README.md` and the workflow table of `AGENTS.md` are inspected
- **THEN** every workflow file they name SHALL exist under `.github/workflows/`
- **AND** every workflow file that exists SHALL be named with the release of the image it runs on

### Requirement: A dependency a workflow installs is named as the release of its image names it

A workflow SHALL install a dependency under the package name the release of its runner image provides
it. When that release keeps the previous name only as a transitional package, the workflow SHALL name
the package the transitional one points to. When a toolchain is part of that release's archive, the
workflow SHALL NOT add a third-party package repository for an older release.

#### Scenario: The test framework is installed under its current package name

- **GIVEN** a Linux job installs the Catch2 development package
- **WHEN** its dependency list is inspected
- **THEN** it SHALL name the package the release provides
- **AND** the configure step of that job SHALL find `Catch2::Catch2WithMain`, so the test suite is built
  instead of being skipped

#### Scenario: The compiler comes from the release of the image

- **GIVEN** a job installs a versioned clang toolchain
- **WHEN** its provisioning steps are inspected
- **THEN** the package SHALL be installable from the archive of the release the image runs on
- **AND** the job SHALL NOT add a package repository pinned to a different release
