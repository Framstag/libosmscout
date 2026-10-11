# Spec Delta

## Purpose

Defines the contract for turning the object data of a set of loaded tiles into the `MapData` of one
render job: which objects the result holds, in which order, at what cost, and how a regression of that
cost is caught. It exists so that the per-pan-step conversion follows the objects a view actually
needs rather than the duplicates the tiles carry, and so that the sequence a render job paints is the
same on every platform.

## ADDED Requirements

### Requirement: Each object of the loaded tiles appears exactly once, with a defined order

Converting the object data of a set of loaded tiles into the `MapData` of one render job SHALL place
every node, way, area and route in the result exactly once, even when several loaded tiles carry the
same object and even when a tile additionally holds objects carried over from parent tiles. An offset
SHALL NOT be treated as identifying an object across different data files: the regular and the
optimized data of a kind are read from different files and their offsets are independent of each
other, so two such objects SHALL both be part of the result. The order of the result SHALL be defined
and reproducible for the same tiles: the objects SHALL be grouped by their source data file in a fixed
order, and SHALL NOT depend on the iteration order of an internal container of the implementation.

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

- **WHEN** the same tile list is converted twice
- **THEN** the resulting sequence of nodes, ways, areas and routes SHALL be identical both times
- **AND** each sequence SHALL be grouped by source data file, and no object of the optimized data file
  of a kind SHALL precede an object of the regular data file of that kind

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

The conversion SHALL measure a phase per source data file it converts - the regular and the optimized
data of the four kinds - and SHALL log one warning per slow phase, naming the phase that was slow and
its duration, so that a slow pan step is attributable from a user device log. The duration a phase may
take before it is reported SHALL be settable, and SHALL default to the value the implementation this
change replaces warned at.

#### Scenario: A phase exceeds its threshold

- **WHEN** a conversion phase takes longer than the threshold of that phase
- **THEN** the system SHALL log one warning that names that phase and reports its duration

#### Scenario: No phase exceeds its threshold

- **WHEN** every phase of a conversion stays below its threshold
- **THEN** the system SHALL NOT log a warning for that conversion

### Requirement: A conversion that became slower fails a test

The conversion SHALL be covered by a test that compares it against a baseline of the conversion as it
was before this change, converting the same tile sets through both, and that fails when the conversion
is slower than the baseline by more than the recorded margins. The test SHALL cover tile sets that
differ in how often their tiles carry the same object, because the cost of a deduplication mechanism
depends on that. The structural cost of the conversion SHALL additionally be pinned by assertions that
do not depend on timing.

#### Scenario: The conversion is slower than the baseline

- **WHEN** the conversion test converts a tile set through the conversion and through the baseline, and
  the conversion takes longer than the baseline by more than the recorded margin of a tile set or more
  than the recorded margin over all tile sets
- **THEN** the test SHALL fail

#### Scenario: The structural cost is pinned without timing

- **WHEN** the structural test converts a tile set, and converts it again with every object carried
  twice
- **THEN** the counted allocated memory blocks SHALL be at most the number of distinct objects of the
  result plus a constant
- **AND** the block count of the conversion of the repeated tile set SHALL NOT exceed the block count of
  the first conversion
- **AND** the assertions SHALL NOT depend on a measured duration

### Requirement: Every conversion entry point of the library applies the same contract

Every entry point of the library that converts loaded tiles into `MapData` SHALL apply the same
uniqueness and order guarantees, including an entry point that restricts the conversion to a set of
object types.

#### Scenario: A restricted conversion

- **WHEN** a conversion is restricted to a set of object types
- **THEN** the result SHALL contain only objects of those types, each of them exactly once
- **AND** the result SHALL be grouped by source data file, and no object of the optimized data file of a
  kind SHALL precede an object of the regular data file of that kind
