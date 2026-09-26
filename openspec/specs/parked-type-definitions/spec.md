# parked-type-definitions Specification

## Purpose
Keeps the shipped type configuration limited to the released type set while
retaining not-yet-released type definitions and their styles in the repository,
so they can be enabled later without being restored from history, and so the
shipped stylesheets always load as a consistent whole.

## Requirements

### Requirement: Shipped type configuration contains only released types

The shipped type configuration SHALL expose exactly the released type set.
Type definitions that are not released yet SHALL NOT be resolvable when the
shipped type configuration is loaded.

#### Scenario: Unreleased types are not resolvable, released types are

- **GIVEN** the shipped type configuration
- **WHEN** it is loaded
- **THEN** unreleased type names such as `historic_aircraft` and
  `landuse_apiary` can not be resolved
- **AND** released type names such as `highway_motorway`, `amenity_restaurant`,
  `shop_supermarket`, `leisure_park` and `historic_castle` can be resolved

#### Scenario: Released type set is unaffected by the retained definitions

- **GIVEN** the shipped type configuration
- **WHEN** the number of resolvable types is compared with the released type
  set
- **THEN** the two match, so retained unreleased definitions add no type to the
  shipped configuration

### Requirement: Unreleased definitions and their styles are retained in inactive form

Every unreleased type definition and every style rule that belongs to an
unreleased type SHALL remain in the repository in an inactive form, so that
enabling the parked set later is a localized edit and not a restoration from
version history.

#### Scenario: Retained definitions and rules are inactive

- **GIVEN** the repository containing the parked set
- **WHEN** the shipped type configuration is loaded and the shipped stylesheets
  are loaded
- **THEN** none of the retained unreleased definitions or their style rules has
  any effect

#### Scenario: Retained definitions are still present in the repository

- **GIVEN** the repository after this change
- **WHEN** the type configuration sources are searched for the unreleased type
  names
- **THEN** each unreleased type definition is found, together with the style
  rules that reference it

### Requirement: Parked state is locatable by one stable marker

The inactive definitions and the inactive style rules SHALL carry the same
stable marker, so that the complete parked set can be located, reviewed and
enabled as one operation.

#### Scenario: Marker count equals the parked set size

- **GIVEN** the repository after this change
- **WHEN** occurrences of the marker are counted in the type configuration and
  in the stylesheets
- **THEN** the count of marked type definitions equals the number of unreleased
  types

#### Scenario: The marker never marks released content

- **GIVEN** the repository after this change
- **WHEN** a released type definition is inspected
- **THEN** it does not carry the marker
- **AND** a marker that sits next to a style rule names only unreleased types

### Requirement: Shipped stylesheets load and resolve every type they reference

Every shipped stylesheet SHALL load successfully against the shipped type
configuration, and no active style rule or group entry may name a type outside
the shipped type set. A reference to a type that does not exist MUST be reported
by the loader rather than silently dropped.

#### Scenario: All shipped stylesheets load

- **GIVEN** the shipped type configuration
- **WHEN** each shipped stylesheet is loaded
- **THEN** loading succeeds for every one of them

#### Scenario: No active style rule references an unreleased type

- **GIVEN** the shipped stylesheets
- **WHEN** the set of types referenced by active style rules and group entries is
  compared with the shipped type configuration
- **THEN** every referenced type is resolvable

#### Scenario: A reference to an inactive type is reported

- **GIVEN** a stylesheet that references an inactive type
- **WHEN** it is loaded against the shipped type configuration
- **THEN** the loader reports the unknown type and the rule does not apply

### Requirement: Types that existed before the change stay fully active

Definitions, styles, patterns and symbols that belong to types which already
existed SHALL keep working. Types that are renamed by this change SHALL be
available under their new name and SHALL no longer be available under the old
name.

#### Scenario: Existing types keep their styles

- **GIVEN** the shipped stylesheets
- **WHEN** a style rule for an existing type such as `highway_motorway` is
  inspected
- **THEN** it is active and applies

#### Scenario: Renamed types are active under the new name only

- **GIVEN** the renamed worship type family
- **WHEN** the shipped type configuration is loaded
- **THEN** `religion_christian` is resolvable and `christian_worship` is not

#### Scenario: Symbols of existing stylesheets are unchanged

- **GIVEN** the shipped motorways stylesheet
- **WHEN** its symbols are enumerated
- **THEN** the symbols of its existing types are reported and no symbol of an
  unreleased type appears

### Requirement: Symbol and pattern inspection covers the active styles only

The symbol and pattern inspection of a loaded stylesheet SHALL report the
symbols and patterns of the active style rules, and SHALL NOT be affected by
retained inactive style rules.

#### Scenario: Patterns of active rules are enumerated

- **GIVEN** a stylesheet whose active rules use two distinct pattern fills
- **WHEN** the patterns are enumerated
- **THEN** both pattern names are reported in a deterministic order

#### Scenario: Parked rules contribute no patterns

- **GIVEN** the shipped stylesheets, in which the rules of unreleased types carry
  the marker
- **WHEN** the patterns of a stylesheet are enumerated
- **THEN** only the patterns of the rules that are active in it are reported
