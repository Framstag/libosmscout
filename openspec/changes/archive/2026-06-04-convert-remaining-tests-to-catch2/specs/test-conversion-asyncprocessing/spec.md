## ADDED Requirements

### Requirement: Async processing test runs under Catch2
The AsyncProcessingTest SHALL use Catch2 TEST_CASE macros and REQUIRE assertions instead of a custom main() with cout-based verification.

#### Scenario: Int worker processes all tasks
- **WHEN** AsyncProcessingTest runs the IntWorker with 4000 tasks and a stop signal
- **THEN** REQUIRE checks that processedCount equals 4000

#### Scenario: Vector worker processes all tasks
- **WHEN** AsyncProcessingTest runs the VectorWorker with 4000 tasks and an empty-vector stop signal
- **THEN** REQUIRE checks that processedCount equals 4000

#### Scenario: Multi-vector worker processes all tasks across 4 workers
- **WHEN** AsyncProcessingTest runs 4 VectorWorkers sharing one queue with 4000 tasks
- **THEN** REQUIRE checks that the sum of all processedCount values equals 4000

#### Scenario: Test name matches original
- **WHEN** CTest runs "Check async processing"
- **THEN** the test executable name is AsyncProcessingTest unchanged