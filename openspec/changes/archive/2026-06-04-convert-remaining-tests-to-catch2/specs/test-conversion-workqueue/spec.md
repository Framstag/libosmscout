## ADDED Requirements

### Requirement: Work queue test runs under Catch2
The WorkQueueTest SHALL use Catch2 TEST_CASE macros and REQUIRE assertions instead of a custom main() with result printing.

#### Scenario: Test processes all pushed tasks
- **WHEN** WorkQueueTest pushes 100 tasks and collects their futures
- **THEN** REQUIRE checks that all 100 futures return the expected sum values

#### Scenario: Test stops cleanly
- **WHEN** WorkQueueTest signals stop after all tasks complete
- **THEN** the queue finishes without error and the test completes successfully

#### Scenario: Test name matches original
- **WHEN** CTest runs "Check implementation of work queue"
- **THEN** the test executable name is WorkQueueTest unchanged