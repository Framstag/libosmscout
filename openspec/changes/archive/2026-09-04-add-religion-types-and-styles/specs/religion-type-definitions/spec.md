## Purpose

Defines the complete set of `religion` type and style definitions in the libosmscout stylesheets so that all relevant `religion`-tagged place-of-worship objects (per the OSM wiki Key:religion page and taginfo usage) are recognized and rendered.

## ADDED Requirements

### Requirement: Religion type definitions cover all documented wiki values
The stylesheet type definitions SHALL include a NODE worship type for every religion value documented on the OSM wiki Key:religion page, except `none` which is discouraged (irreligious use). The following values SHALL be added: `jewish`, `muslim`, `buddhist`, `hindu`, `shinto`, `taoist`, `sikh`, `jain`, `pagan`, `zoroastrian`, `chinese_folk`, `multifaith`, `bahai`, `confucian`, `vietnamese_folk`, `ancestor`, `animist`, `antoinist`, `benzhu`, `caodaism`, `shamanic`, `scientologist`, `self-realization_fellowship`, `spiritualist`, `tenrikyo`, `unitarian_universalist`, `voodoo`, `yazidi`.

#### Scenario: New religion value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `religion=buddhist` with `amenity=place_of_worship`
- **THEN** a `buddhist_worship` type definition exists in the Religious section of `map.ost`

#### Scenario: Discouraged value is excluded
- **WHEN** a stylesheet consumer looks up the type for tag `religion=none`
- **THEN** no dedicated worship type definition exists for it

### Requirement: Object types match OSM wiki documentation
Each new religion worship type SHALL be declared for NODE objects only, matching the OSM wiki documentation that religion values may be used on nodes and areas; areas are covered by the existing generic `temple_building`, `shrine_building`, and `worship_building` types.

#### Scenario: Worship node has a type
- **WHEN** a stylesheet consumer loads the type for a node tagged `amenity=place_of_worship` + `religion=shinto`
- **THEN** the `shinto_worship` type is declared for NODE objects

#### Scenario: Worship area without building falls back to generic amenity
- **WHEN** a stylesheet consumer loads the type for an area tagged `amenity=place_of_worship` + `religion=hindu` without a `building` tag
- **THEN** the area is matched by the generic `amenity` type rather than a religion-specific worship type

### Requirement: Existing muslim mosque type matches the correct religion value
The `muslim_mosque_building` type definition SHALL match `religion=muslim` (the value documented on the OSM wiki), not the misspelled `muslin`.

#### Scenario: Muslim mosque building is matched
- **WHEN** a stylesheet consumer loads the type for an area tagged `amenity=place_of_worship` + `religion=muslim` + `building=mosque`
- **THEN** the `muslim_mosque_building` type matches it

### Requirement: Style definitions for new religion types
Each new religion worship type SHALL have a corresponding style definition in `include/religious.oss` where visualization is obvious, reusing the existing christian worship style (label + icon at very close zoom) as template. Symbols SHALL be named with a `religion_` prefix.

#### Scenario: Worship node is labeled and iconed
- **WHEN** a map is rendered at very close zoom with a `religion=buddhist` worship node
- **THEN** the node is drawn with a label and the `religion_buddhist_dharma_wheel` icon

#### Scenario: Major religions have distinct symbols
- **WHEN** a map is rendered at very close zoom with worship nodes for `jewish`, `muslim`, `taoist`, `shinto`, and `pagan`
- **THEN** each node is drawn with its own symbol (`religion_jewish_star_of_david`, `religion_muslim_crescent`, `religion_taoist_yin_yang`, `religion_shinto_torii`, `religion_pagan_pentagram`)

#### Scenario: Remaining religions use a generic symbol
- **WHEN** a map is rendered at very close zoom with a `religion=hindu` worship node
- **THEN** the node is drawn with the generic `religion_place_of_worship` symbol

### Requirement: Existing religion types and styles remain unchanged
The existing type definitions for `temple_building`, `shrine_building`, `christian_church_building`, `christian_cathedral_building`, `christian_chapel_building`, `christian_worship`, `jewish_synagogue_building`, and `worship_building` SHALL keep their current names and behavior; the change SHALL only add new definitions and fix the `muslim_mosque_building` typo.

#### Scenario: Existing types still resolve
- **WHEN** a stylesheet consumer loads the type for `religion=christian` + `building=church`
- **THEN** the existing `christian_church_building` type definition is still present and unchanged
