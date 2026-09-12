## MODIFIED Requirements

### Requirement: Religion type definitions cover all documented wiki values
The stylesheet type definitions SHALL include a NODE worship type for every religion value documented on the OSM wiki Key:religion page, except `none` which is discouraged (irreligious use). The following values SHALL be added: `jewish`, `muslim`, `buddhist`, `hindu`, `shinto`, `taoist`, `sikh`, `jain`, `pagan`, `zoroastrian`, `chinese_folk`, `multifaith`, `bahai`, `confucian`, `vietnamese_folk`, `ancestor`, `animist`, `antoinist`, `benzhu`, `caodaism`, `shamanic`, `scientologist`, `self-realization_fellowship`, `spiritualist`, `tenrikyo`, `unitarian_universalist`, `voodoo`, `yazidi`.

#### Scenario: New religion value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `religion=buddhist` with `amenity=place_of_worship`
- **THEN** a `religion_buddhist` type definition exists in the Religious section of `map.ost`

#### Scenario: Discouraged value is excluded
- **WHEN** a stylesheet consumer looks up the type for tag `religion=none`
- **THEN** no dedicated worship type definition exists for it

### Requirement: Object types match OSM wiki documentation
Each new religion worship type SHALL be declared for NODE objects only, matching the OSM wiki documentation that religion values may be used on nodes and areas; areas are covered by the existing generic `religion_temple_building`, `religion_shrine_building`, and `religion_building` types.

#### Scenario: Worship node has a type
- **WHEN** a stylesheet consumer loads the type for a node tagged `amenity=place_of_worship` + `religion=shinto`
- **THEN** the `religion_shinto` type is declared for NODE objects

#### Scenario: Worship area without building falls back to generic amenity
- **WHEN** a stylesheet consumer loads the type for an area tagged `amenity=place_of_worship` + `religion=hindu` without a `building` tag
- **THEN** the area is matched by the generic `amenity` type rather than a religion-specific worship type

### Requirement: Existing muslim mosque type matches the correct religion value
The `religion_muslim_mosque_building` type definition SHALL match `religion=muslim` (the value documented on the OSM wiki), not the misspelled `muslin`.

#### Scenario: Muslim mosque building is matched
- **WHEN** a stylesheet consumer loads the type for an area tagged `amenity=place_of_worship` + `religion=muslim` + `building=mosque`
- **THEN** the `religion_muslim_mosque_building` type matches it

## ADDED Requirements

### Requirement: Religion types follow the religion_ naming scheme
All religion type definitions SHALL follow the general `<key>_<value>` naming scheme with the `religion_` prefix. Worship node types SHALL be named `religion_<value>` (e.g. `religion_christian`, `religion_buddhist`, `religion_yazidi`). Building types SHALL be named `religion_<value>_<building>_building` (e.g. `religion_christian_church_building`, `religion_jewish_synagogue_building`, `religion_muslim_mosque_building`) or `religion_<building>_building` for generic building types (`religion_temple_building`, `religion_shrine_building`, `religion_building`). The old `<religion>_worship` and `<religion>_<building>_building` names SHALL no longer exist.

#### Scenario: Worship types use the religion_ prefix
- **WHEN** a stylesheet consumer looks up the type for a node tagged `amenity=place_of_worship` + `religion=christian`
- **THEN** the `religion_christian` type definition exists
- **AND** no `christian_worship` type definition exists

#### Scenario: Building types use the religion_ prefix
- **WHEN** a stylesheet consumer looks up the type for an area tagged `amenity=place_of_worship` + `religion=christian` + `building=church`
- **THEN** the `religion_christian_church_building` type definition exists
- **AND** no `christian_church_building` type definition exists

#### Scenario: Generic building types use the religion_ prefix
- **WHEN** a stylesheet consumer looks up the type for an area tagged `amenity=place_of_worship` with a generic `building` tag
- **THEN** the `religion_building` type definition exists
- **AND** no `worship_building` type definition exists

## REMOVED Requirements

### Requirement: Existing religion types and styles remain unchanged
**Reason**: Superseded by the rename to the `religion_` naming scheme; the types this requirement protected are now renamed.
**Migration**: All religion types now follow the `religion_` prefix scheme; see "Religion types follow the religion_ naming scheme".
