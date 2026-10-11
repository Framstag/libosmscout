# Spec Delta

## Purpose

The decision which node ids keep their serial during the import of areas and ways: the rule and its
exceptions, the temporary files the decision is applied to, and the cost at which the import reaches
that decision.

## ADDED Requirements

### Requirement: Node serial clearing rule

The id optimization step SHALL clear the serial of a node id that is referenced by exactly one ring or
one way that is relevant for routing, and SHALL keep the serial of a node id that is referenced by
more than one of them. References of rings and ways whose type is not routable SHALL NOT count towards
that decision, while the serials of the nodes of those objects SHALL follow the same global decision
as any other object. The step SHALL apply the decision to every area and every way it writes, so that
the outcome does not depend on the order in which the areas and the ways are examined.

#### Scenario: An id referenced once is cleared

- **WHEN** a node id is referenced by exactly one routable ring or way of the input
- **THEN** the serial of that node id SHALL be cleared in the written object
- **AND** the number of cleared serials SHALL match the number of references to such ids

#### Scenario: An id referenced by two objects keeps its serial

- **WHEN** a node id is referenced by two different routable rings or ways of the input
- **THEN** the serial of that node id SHALL be kept

#### Scenario: An id shared between an area and a way keeps its serial

- **WHEN** a node id is referenced once by a routable area ring and once by a routable way
- **THEN** the serial of that node id SHALL be kept, whichever of the two is examined first

#### Scenario: An id repeated inside one ring counts once

- **WHEN** a ring references the same node id more than once and no other routable object references
  that id
- **THEN** the id SHALL count as referenced once
- **AND** its serial SHALL be cleared in that ring

#### Scenario: An id repeated inside one non-circular way counts once

- **WHEN** a way that is not circular references the same node id more than once and no other routable
  object references that id
- **THEN** the id SHALL count as referenced once
- **AND** its serial SHALL be cleared in that way

#### Scenario: A circular way keeps the id of its first node

- **WHEN** a routable way is circular
- **THEN** the serial of the node id it returns to SHALL be kept, even when no other object references
  it

#### Scenario: An id of a non-routable object alone is cleared

- **WHEN** a node id is referenced only by objects whose type is not routable
- **THEN** the id SHALL count as unreferenced for the decision
- **AND** its serial SHALL be cleared in every object that carries it

### Requirement: The provided files are unchanged

The two temporary files the step provides SHALL be byte-identical to the files the implementation
before this change writes for the same input: the areas file, including the ring centre each area
carries, and the ways file. The step SHALL remain a step of the import pipeline that requires and
provides the same files as before, and SHALL leave the objects it does not decide about untouched.

#### Scenario: The same input produces identical files

- **WHEN** the step runs over a given `areas2.tmp` and `wayway.tmp` and the same step of the previous
  implementation runs over the same input
- **THEN** the produced `areas3.tmp` SHALL be byte-identical
- **AND** the produced `ways.tmp` SHALL be byte-identical

#### Scenario: A ring whose centre is recorded keeps that centre

- **WHEN** an area ring of the input receives a centre when it is written
- **THEN** the written centre SHALL be the same value the previous implementation recorded

### Requirement: The cost of the decision follows the distinct ids, not the processed objects

The cost of the decision SHALL be bounded by the number of distinct node ids of the input and by the
objects the step writes, and SHALL NOT include a container that exists for one ring or one way and
grows with it. The step SHALL examine each of its inputs only as often as the decision and the written
output require, and SHALL NOT make a further pass over an input after the output has been produced.

#### Scenario: The allocation does not grow with the object count

- **WHEN** the step runs over an input whose rings and ways reference the same set of distinct node ids
  but whose object count is ten times higher
- **THEN** the number of allocated memory blocks SHALL be bounded by the number of distinct node ids
  plus the number of written objects, and SHALL NOT grow by a container per processed object

#### Scenario: An input is read once per phase that needs it

- **WHEN** the step runs over its two inputs
- **THEN** each input SHALL be opened and parsed once to collect ids and once to write the objects it
  provides, and no further time after the output has been written

### Requirement: The cost is measured against the recorded baseline

The step SHALL report its duration and its peak resident memory in the import log, and the change SHALL
record both for the recorded baseline extract next to the baseline they are compared against. A run of
the step that is slower than the baseline for the same extract SHALL be visible from the log and from
the recorded comparison.

#### Scenario: The step reports its cost

- **WHEN** the step finishes
- **THEN** the import log SHALL contain the duration and the peak resident memory of the step

#### Scenario: The comparison is recorded

- **WHEN** the change is complete
- **THEN** the duration and the peak resident memory of the step for the baseline extract SHALL be
  recorded together with the baseline values from `maps/Dortmund.txt` step #11
- **AND** a step that is not faster than the baseline SHALL be reported as an unmet goal of the change
  rather than as a completed improvement

### Requirement: The decision rule is observable on its own

The rule that decides which ids count as referenced at least twice SHALL be reachable from a unit test
without running the whole step, so that a test can pin the rule for objects that the step's own inputs
rarely contain (an id repeated inside one ring, an id repeated inside a non-circular way, an id shared
between an area and a way, the first id of a circular way).

#### Scenario: The rule is tested without the pipeline

- **WHEN** the unit tests of the import library run
- **THEN** a test SHALL drive the rule through input objects and assert which ids it reports as
  referenced at least twice, without reading or writing an import temporary file

#### Scenario: The rule and the step agree

- **WHEN** the same objects pass through the rule in a unit test and through the step in a fixture test
- **THEN** the cleared serials SHALL be the same in both
