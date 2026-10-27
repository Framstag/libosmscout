# location-index-lookup Specification

## Purpose
Defines what a lookup on the location index of a database costs relative to the region entries it visits,
so that reading the region tree of `location.idx` follows what a lookup asks of it rather than the number of
regions the index holds.

## Requirements

### Requirement: A lookup deserializes a region entry once while the index retains it

A lookup on the location index SHALL deserialize each region entry it visits at most once while the index
retains it: a traversal that visits region entries the index already deserialized from `location.idx` SHALL
read none of them from the file again. A traversal SHALL report the region entries it visits — their
offsets, names and order — independently of whether the index read them or retained them.

#### Scenario: A second traversal of the same tree reads no region entry

- **GIVEN** an opened database whose location index carries a region tree
- **WHEN** the complete region tree is traversed twice, one traversal after the other
- **THEN** the first traversal SHALL read the region entries of the regions it visits
- **AND** the second traversal SHALL read no region entry
- **AND** it SHALL report the same regions, in the same order, as the first

Test case: `LocationIndexRegionCacheTest` — "A second traversal of the region tree reads no region entry"

### Requirement: A flush releases the retained region entries

The region entries a lookup retains SHALL be released by a flush of the index. After a flush, a traversal
SHALL read every region entry it visits from the index file again, so the memory the index retains follows
what it deserialized since that flush.

#### Scenario: A traversal after a flush reads the regions it visits

- **GIVEN** a location index whose region tree was traversed once and then flushed
- **WHEN** the complete region tree is traversed again
- **THEN** the traversal SHALL read exactly the region entries of the regions it visits, one per region
- **AND** it SHALL report the same regions, in the same order, as the traversal before the flush

Test case: `LocationIndexRegionCacheTest` — "A traversal after a flushed cache reads the regions it visits"
