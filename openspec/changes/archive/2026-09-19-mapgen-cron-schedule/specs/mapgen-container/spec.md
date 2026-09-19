# Spec Delta

## MODIFIED Requirements

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

## ADDED Requirements

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
