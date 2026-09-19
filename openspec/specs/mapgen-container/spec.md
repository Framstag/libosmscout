# Mapgen Container Specification

## Purpose

Container image that bundles the regeneration process with a pinned version of the import tool, can mount the database repository to update, and runs one complete pass per invocation under an external scheduler.

## Requirements

### Requirement: Image contents
The container image SHALL contain the regeneration process, the import tool with its runtime dependencies, and the utilities needed to download and verify source data. The image SHALL identify the exact import tool version it bundles.

#### Scenario: Self-contained image
- **WHEN** the image is started with the required mounts
- **THEN** the full regeneration process including import runs inside the container without host-provided tooling

#### Scenario: Tool version identifiable
- **WHEN** an operator inspects the image
- **THEN** the bundled import tool version is determinable and matches the version recorded in generated database metadata

### Requirement: Entry point semantics
The image entry point SHALL either execute one complete pass over the imports manifest, honoring the refresh
frequency gates, and exit when the pass is done, or - when a schedule is configured - run the pass on that
schedule until the container is stopped. With no schedule configured the image SHALL NOT run an internal
scheduler; in the scheduled mode the image SHALL contain a scheduler that needs no root privileges and no
writable root filesystem. Arguments given to the container SHALL be passed to the pass itself, which runs it
once and exits, regardless of a configured schedule.

#### Scenario: One pass per start
- **WHEN** the container is started without a schedule configured
- **THEN** it checks due imports, performs applicable downloads and imports, and exits with a status reflecting the outcome

#### Scenario: No scheduler inside
- **WHEN** the container runs without a schedule configured
- **THEN** it terminates after the pass and does not schedule further work itself

#### Scenario: Arguments run the pass once
- **WHEN** the container is started with arguments, for example a configuration check
- **THEN** it runs the pass with those arguments, exits, and does not start the scheduled mode

### Requirement: Volume mounts
The image SHALL mount three distinct areas: a transient work area for downloads and intermediate import data, the database repository to update, and the configuration holding the imports manifest read-only. The region index used for layout is not part of the configuration; the script reads it from the served root of the repository (`public/names.json`).

#### Scenario: Work area isolated
- **WHEN** an import runs in the container
- **THEN** downloads and intermediate data are confined to the transient work area

#### Scenario: Configuration read-only
- **WHEN** the regeneration process reads configuration
- **THEN** the configuration area is not writable by the process

#### Scenario: Repository updated in place
- **WHEN** a database is placed or pruned
- **THEN** this happens in the mounted database repository

### Requirement: Least privilege
The container SHALL run the process as a non-root user and SHALL use a read-only root filesystem, with writability limited to the mounted areas.

#### Scenario: Non-root execution
- **WHEN** the container starts
- **THEN** the process runs without root privileges

#### Scenario: Read-only root filesystem
- **WHEN** the container starts
- **THEN** the image's own filesystem is not writable by the process

### Requirement: Configurable runtime identity

The orchestration of the regeneration container SHALL let the operator determine the uid and gid that the
regeneration pass runs as, through the environment, defaulting to the identity the image documents (uid 1000,
gid 1000) when nothing is set. The mounted areas SHALL be written with that identity, so that a repository
mounted from a directory owned by the operator's own account is usable without changing ownership. The image
SHALL create its non-root user with the same documented numeric ids. The serving container SHALL keep the
identity of its base image.

#### Scenario: The default identity is the documented one

- **GIVEN** an orchestration run with no identity configured
- **WHEN** the regeneration pass starts
- **THEN** it SHALL run as uid 1000 and gid 1000
- **AND** the image's own non-root user SHALL have that same uid and gid

#### Scenario: The operator chooses the identity of the mounted areas

- **GIVEN** a repository area owned by an account other than 1000
- **WHEN** the operator configures that account's uid and gid and starts a pass
- **THEN** the pass SHALL run with that identity
- **AND** files it creates in the mounted areas SHALL carry that identity, needing no ownership change on
  the host side

#### Scenario: A configured identity still requires write access

- **GIVEN** a repository area that the configured identity may not write to
- **WHEN** a pass runs
- **THEN** it SHALL fail with the error of the underlying write, and SHALL NOT fall back to another identity

#### Scenario: The serving container is unaffected

- **GIVEN** the serving container
- **WHEN** it runs with an identity configured for the regeneration container
- **THEN** it SHALL keep the identity of its base image and continue to serve the repository read-only

### Requirement: Scheduled operation

When a schedule is configured, the image SHALL run one pass per occurrence of that schedule until the
container is stopped, taking the schedule from an environment variable that holds a cron expression. The
scheduled mode SHALL work as the image's non-root user with a read-only root filesystem. It SHALL refuse to
start when the expression is unusable, rather than starting and running nothing. Two passes SHALL NOT overlap:
an occurrence that arrives while a pass is still running SHALL be skipped and reported, whether the running
pass came from the schedule or from an external trigger.

#### Scenario: The schedule runs the pass
- **GIVEN** a container started with a schedule configured
- **WHEN** an occurrence of that schedule arrives
- **THEN** a pass runs and its output appears in the container log, and the container keeps running for the
  next occurrence

#### Scenario: An unusable expression stops the start
- **GIVEN** a container started with a schedule that is not a valid cron expression
- **WHEN** it starts
- **THEN** it exits with a status reporting the failure and runs no pass

#### Scenario: Overlapping passes are skipped
- **GIVEN** a pass that is still running
- **WHEN** the next occurrence of the schedule arrives, or an external trigger starts another pass
- **THEN** the second pass does not run, is reported as skipped, and the running pass is not disturbed

#### Scenario: The scheduled mode needs no privileges
- **GIVEN** the image's non-root user and a read-only root filesystem
- **WHEN** the scheduled mode runs
- **THEN** it starts and runs passes without writing outside the mounted areas and without root privileges
