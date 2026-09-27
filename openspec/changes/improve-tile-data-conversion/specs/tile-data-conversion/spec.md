# Spec Delta

## Purpose

Defines the contract for turning the object data of a set of loaded tiles into the `MapData` of one
render job: which objects the result holds, in which order, at what cost, and how a regression of that
cost is caught. It exists so that the per-pan-step conversion follows the objects a view actually
needs rather than the duplicates the tiles carry, and so that the sequence a render job paints is the
same on every platform.

## ADDED Requirements

### Requirement: Each object of the loaded tiles appears exactly once, in an order defined by the database

Converting the object data of a set of loaded tiles into the `MapData` of one render job SHALL place
every node, way, area and route in the result exactly once, even when several loaded tiles carry the
same object and even when a tile additionally holds objects carried over from parent tiles. The
objects of the result SHALL be ordered by the data file they were read from and, within one data file,
ascending by their offset in it, so that the sequence a render job receives does not depend on the
platform, the standard library or the order in which the tiles were handed in. An offset SHALL NOT be
treated as identifying an object across different data files.

#### Scenario: An object carried by several loaded tiles appears once

- **WHEN** two or more loaded tiles contain the same node, way, area or route
- **THEN** the converted result SHALL contain that object exactly once

#### Scenario: An object carried over from a parent tile and loaded by the tile itself appears once

- **WHEN** a tile holds an object both in the data carried over from parent tiles and in the data it
  loaded itself
- **THEN** the converted result SHALL contain that object exactly once

#### Scenario: Objects of two data files that share an offset both appear

- **WHEN** a loaded tile holds an object read from the regular data file and an object read from the
  optimized data file of the same kind, and both objects carry the same offset value
- **THEN** the converted result SHALL contain both objects

#### Scenario: The resulting sequence is reproducible

- **WHEN** the same tile set is converted twice, with the tiles handed in in a different order
- **THEN** the resulting sequence of nodes, ways, areas and routes SHALL be identical both times
- **AND** each sequence SHALL be grouped by source data file and ascending by file offset within each
  source

### Requirement: The cost of the conversion follows the distinct objects, not the objects of all tiles

The heap allocation of the conversion SHALL be bounded by the number of distinct objects of the
result, and SHALL NOT grow with the number of loaded tiles that carry those objects. Every object
stored in a tile SHALL be examined once per conversion; the conversion SHALL NOT make a further pass
over the tile data after the result has been assembled.

#### Scenario: A tile set whose tiles share most objects

- **WHEN** a tile set is converted whose tiles carry the same objects in several of them
- **THEN** the number of allocated memory blocks SHALL be bounded by the number of distinct objects of
  the result plus a constant
- **AND** that number SHALL NOT grow with the number of tiles that carry the duplicates

#### Scenario: Each stored object is read once

- **WHEN** a tile set is converted
- **THEN** every object stored in the tiles SHALL be read exactly once
- **AND** the conversion SHALL NOT traverse the stored tile data again after assembling the result

### Requirement: Every phase of the conversion reports itself when it is slow

The conversion SHALL measure its phases separately and SHALL log one warning per slow phase, naming
the phase that was slow and its duration, so that a slow pan step is attributable from a user device
log.

#### Scenario: A phase exceeds its threshold

- **WHEN** a conversion phase takes longer than the threshold of that phase
- **THEN** the system SHALL log one warning that names that phase and reports its duration

#### Scenario: No phase exceeds its threshold

- **WHEN** every phase of a conversion stays below its threshold
- **THEN** the system SHALL NOT log a warning for that conversion

### Requirement: A conversion that became slower fails a test

The conversion SHALL be covered by a test that compares it against a recorded baseline of the
conversion as it was before this change, converting the same tile set through both, and that fails
when the conversion is slower than the baseline by more than the recorded margin. The structural cost
of the conversion SHALL additionally be pinned by an assertion that does not depend on timing.

#### Scenario: The conversion is slower than the baseline

- **WHEN** the conversion test converts its tile set through the conversion and through the recorded
  baseline, and the conversion takes longer than the baseline by more than the recorded margin
- **THEN** the test SHALL fail

#### Scenario: The structural cost is pinned without timing

- **WHEN** the structural test converts a generated tile set with a known duplicate ratio
- **THEN** the counted allocated memory blocks SHALL be at most the number of distinct objects of the
  result plus a constant
- **AND** the assertion SHALL NOT depend on a measured duration

### Requirement: Every conversion entry point of the library applies the same contract

Every entry point of the library that converts loaded tiles into `MapData` SHALL apply the same
uniqueness and order guarantees, including an entry point that restricts the conversion to a set of
object types.

#### Scenario: A restricted conversion

- **WHEN** a conversion is restricted to a set of object types
- **THEN** the result SHALL contain only objects of those types, each of them exactly once
- **AND** the result SHALL be grouped by source data file and ascending by file offset within each
  source
