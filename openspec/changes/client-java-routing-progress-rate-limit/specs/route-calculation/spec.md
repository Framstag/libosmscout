# Spec Delta

## Purpose

JNI bridge to libosmscout routing engine for async route calculation between two coordinates.

## ADDED Requirements

### Requirement: Progress reports to Java are throttled and moved-driven

While an asynchronous route calculation runs, the bridge SHALL hand a progress value to Java only when the
reported percentage has changed and at most once per documented rate-limit interval. The first report of a
calculation SHALL be handed over immediately. The reported percentage SHALL be non-decreasing for one
calculation and SHALL be capped below completion, completion being delivered as the calculation's result.

#### Scenario: An unchanged percentage is not reported again

- **GIVEN** a running route calculation whose progress has not moved since the last report
- **WHEN** the routing engine reports progress for further edges
- **THEN** no further progress callback SHALL be made to Java

#### Scenario: A report inside the rate-limit interval is dropped

- **GIVEN** a progress report that was handed to Java
- **WHEN** the percentage changes again inside the rate-limit interval
- **THEN** that change SHALL NOT be handed to Java
- **AND** a later change after the interval SHALL be handed to Java

#### Scenario: The first report is not delayed

- **GIVEN** a route calculation that has just started reporting progress
- **WHEN** the first percentage becomes available
- **THEN** it SHALL be handed to Java without waiting for the rate-limit interval

#### Scenario: Progress never reports completion

- **GIVEN** a running route calculation whose progress approaches the whole route
- **WHEN** progress is reported
- **THEN** the reported percentage SHALL NOT be the completed value
- **AND** completion SHALL be delivered as the calculation's result

#### Scenario: Progress does not go backwards

- **GIVEN** a running route calculation
- **WHEN** successive progress reports are handed to Java
- **THEN** each reported percentage SHALL be at least the previously reported one

#### Scenario: A long calculation still reports progress

- **GIVEN** a route long enough that its calculation takes longer than the rate-limit interval
- **WHEN** the calculation runs
- **THEN** Java SHALL receive more than one progress report
- **AND** the reports SHALL describe a growing percentage
