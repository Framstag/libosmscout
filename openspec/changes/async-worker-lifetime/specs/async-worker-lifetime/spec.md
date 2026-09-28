# Spec Delta

## Purpose

Defines how a worker that owns a job queue shuts down and what happens to the work that is in flight or queued when it does, so that destroying an object can never leave a job running against state that no longer exists, so that a job which fails is reported instead of ending the process, and so that a teardown of a long job is bounded by the stop request the job received.

## ADDED Requirements

### Requirement: A worker stops its job thread before the state its jobs use is destroyed

Destroying a worker SHALL stop its job thread and wait for the job that is currently running, and it SHALL do so before the state that its jobs read is destroyed. This SHALL hold when the job is reading a member of the object being destroyed and when the job holds a lock on one of its members. Stopping a worker SHALL stop the queue and drop the jobs that have not started in one step, so that a job which is returning at that moment cannot pick one of them up: such a job SHALL NOT run, and the caller waiting for it SHALL be released instead of waiting forever. A job that polls the `Breaker` it was given SHALL stop at its next check, which SHALL bound the wait; a job that does not poll SHALL still be waited for.

#### Scenario: Destroying a worker with a job in progress

- **GIVEN** a worker whose job is running and reading a member of the worker
- **WHEN** the worker is destroyed
- **THEN** the destruction SHALL wait for that job to finish
- **AND** the job SHALL observe the member in its valid state, not a destroyed one
- **AND** no allocation or access SHALL be performed against destroyed state

#### Scenario: Destroying a worker with a job that is still queued

- **GIVEN** a worker with a job that was submitted and has not started
- **WHEN** the worker is destroyed
- **THEN** that job SHALL NOT run
- **AND** the destruction SHALL complete without waiting for it
- **AND** its caller SHALL be released, so that it does not wait for a job that never runs

#### Scenario: A job that observes the stop request does not delay the teardown

- **GIVEN** a worker whose job polls the `Breaker` it received and would otherwise keep running
- **WHEN** the worker is destroyed
- **THEN** the job SHALL end at its next check
- **AND** the destruction SHALL not wait for the remaining work of that job

#### Scenario: A job that ignores the stop request is still waited for

- **GIVEN** a worker whose job does not poll its `Breaker`
- **WHEN** the worker is destroyed
- **THEN** the destruction SHALL wait for that job to finish
- **AND** the state the job uses SHALL still be alive while it runs

#### Scenario: A worker that is destroyed from its own job thread

- **GIVEN** a worker that deletes itself from within one of its jobs, after it stopped accepting jobs
- **WHEN** it is destroyed
- **THEN** the destruction SHALL complete without the worker waiting for itself

#### Scenario: Shutting a worker down twice

- **GIVEN** a worker that has already been shut down
- **WHEN** it is shut down again, as happens when a derived class and the base class both do it
- **THEN** the second shutdown SHALL be harmless

### Requirement: A failing job is reported and does not end the process

A job that throws SHALL NOT terminate the process and SHALL NOT stop the worker from serving the jobs that follow it. The failure SHALL be reported through the library's logging, the worker SHALL keep running, and a caller waiting for the failed job SHALL be released.

#### Scenario: A job throws

- **GIVEN** a worker with a job that throws
- **WHEN** the job runs
- **THEN** the failure SHALL be logged
- **AND** the process SHALL keep running

#### Scenario: The worker keeps serving its queue after a failure

- **GIVEN** a worker whose first job failed
- **WHEN** a further job is submitted
- **THEN** that job SHALL run and its result SHALL be reported to its caller

#### Scenario: The caller of a failed job is released

- **GIVEN** a job that throws, and a caller waiting for that job
- **WHEN** the job fails
- **THEN** the caller SHALL be released with the default value of the job's result type
- **AND** SHALL NOT wait forever
