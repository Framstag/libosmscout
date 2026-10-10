# Spec Delta

## Purpose

Executing the JavaScout test suite yields a result that names the class at fault, does not depend on the
order in which classes run, and reports an outcome for every class it started.

## ADDED Requirements

### Requirement: Test classes do not share native client state

A test run SHALL NOT let native client state obtained by one test class change the outcome of another
test class. A test that has obtained a native client SHALL release it however that test ends -
normally, with a failed assertion, or with a skipped assumption.

#### Scenario: A skipped test releases the client it obtained

- **GIVEN** a test that obtains a native client and then skips itself
- **WHEN** the test ends
- **THEN** a test that runs afterwards SHALL be able to obtain a native client

#### Scenario: A failing test releases the client it obtained

- **GIVEN** a test that obtains a native client and then fails an assertion
- **WHEN** the test ends
- **THEN** a test that runs afterwards SHALL be able to obtain a native client

#### Scenario: A class outcome does not depend on the classes that ran before it

- **GIVEN** a test class whose client is built and abandoned, as a fault injection placed immediately
  before the class under observation in the run order
- **WHEN** the observed class runs
- **THEN** the observed class SHALL obtain a native client
- **AND** its tests SHALL report the outcomes they report without the fault injection present

#### Scenario: A class that cannot obtain a client reports its tests as skipped

- **GIVEN** a test class whose native client cannot be obtained
- **WHEN** the class runs
- **THEN** its tests SHALL be reported as skipped with the reason it could not obtain the client
- **AND** no later class SHALL be affected

### Requirement: A test asserts the precondition it depends on

A test whose assertion depends on an initialised native client SHALL establish that precondition before
asserting the behaviour under test, so that a client which is not initialised cannot satisfy the
assertion in place of the behaviour the test claims.

#### Scenario: An uninitialised client cannot satisfy an assertion

- **GIVEN** a test that asserts the result of a native client operation
- **WHEN** the client the test uses is not initialised
- **THEN** the test SHALL fail rather than report the asserted result

#### Scenario: A claim about an invalid argument holds on an initialised client

- **GIVEN** a test that claims a client operation yields no result for an invalid argument
- **WHEN** the test runs
- **THEN** it SHALL have established that its client is initialised
- **AND** the asserted absence of a result SHALL come from the invalid argument

### Requirement: An execution reports every test class it started

An execution of the suite SHALL report an outcome for every test class it started, whether that outcome
is passed, failed or skipped.

#### Scenario: Every started class appears in the report

- **GIVEN** an execution of the whole suite
- **WHEN** the execution ends
- **THEN** every test class in the suite SHALL appear in the run's report with an outcome

#### Scenario: An execution does not end with an unreported class

- **GIVEN** an execution of the whole suite
- **WHEN** the execution ends
- **THEN** it SHALL NOT have left a test class it started without an outcome

### Requirement: Only test cases are executed as tests

A source in the test tree that provides a manual entry point instead of test cases SHALL NOT be
executed as a test and SHALL NOT be reported as a test class.

#### Scenario: A manual entry point is not run by the suite

- **GIVEN** a test tree source that provides a manual entry point rather than test cases
- **WHEN** the suite executes
- **THEN** that source SHALL NOT be executed
- **AND** it SHALL NOT appear in the run's report

### Requirement: An abnormally ended run is reported as such

An execution that ends because the test process terminated abnormally SHALL be reported as an
abnormally ended run, distinguishable from an execution that ended with failing assertions, and the
report SHALL name the class that was executing.

#### Scenario: An abnormal end is not reported as a failing test

- **GIVEN** an execution in which the test process terminates abnormally
- **WHEN** the execution ends
- **THEN** the outcome SHALL be reported as an abnormally ended run
- **AND** the report SHALL name the test class that was executing

#### Scenario: The verdict on the motivating abort is evidence-based

- **GIVEN** the abort that motivated this change
- **WHEN** the suite is executed repeatedly against a native library built from the current revision
- **THEN** the change SHALL record whether the abort still occurs
- **AND** the record SHALL state how many executions were made and which revision the library was
  built from
