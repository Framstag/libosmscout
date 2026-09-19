# piste-type-definitions Specification

## Purpose

Defines the complete set of `piste:type` type and style definitions in the libosmscout stylesheets so that all relevant winter-sport piste objects (per the OSM wiki Pistes page and taginfo usage) are recognized and rendered.

## ADDED Requirements

### Requirement: Piste type definitions cover all wiki-documented values
The stylesheet type definitions SHALL include a type for every `piste:type` value documented on the OSM wiki Pistes page: `downhill`, `nordic`, `skitour`, `hike`, `sled`, `sleigh`, `ice_skate`, `snow_park`, `playground`, `ski_jump`, `fatbike`, `snowkite`, `snowmobile`, and `man_made=piste:halfpipe`. The `downhill` value SHALL be split into difficulty variants (`novice`, `easy`, `intermediate`, `advanced`, `expert`, `freeride`, `extreme`) plus a generic fallback for downhill ways without a difficulty tag.

#### Scenario: Wiki-documented value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=nordic`
- **THEN** a type definition exists for it in the winter sports section of `map.ost`

#### Scenario: Downhill difficulty variants exist
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=downhill` with `piste:difficulty=novice`
- **THEN** a type definition `piste_downhill_novice` exists, and the difficulty-specific types are declared before the generic `piste_downhill` fallback so the first match wins

#### Scenario: Halfpipe value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `man_made=piste:halfpipe`
- **THEN** a type definition exists for it

### Requirement: Taginfo-driven types cover relevant additional values
The stylesheet type definitions SHALL include a type for every additional `piste:type` value with at least 150 uses on taginfo that is not discouraged: `connection` (approved status), `snowshoe`, and the semicolon combinations `nordic;hike`, `hike;fatbike`, `hike;snowshoe`, `nordic;hike;snowshoe`, `nordic;fatbike`. Order-variant combinations (`hike;nordic`, `fatbike;hike`) SHALL be covered by the same type as their canonical form.

#### Scenario: Approved value has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=connection`
- **THEN** a type definition exists for it

#### Scenario: Semicolon combination has a type definition
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=nordic;hike`
- **THEN** a type definition exists for it

#### Scenario: Order-variant combination is covered
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=hike;nordic`
- **THEN** the same type definition as for `nordic;hike` matches

#### Scenario: Discouraged values are excluded
- **WHEN** a stylesheet consumer looks up the type for tag `piste:type=yes`
- **THEN** no type definition exists for it

### Requirement: Object types match OSM wiki documentation
Each new piste type SHALL be declared with the object types (NODE, WAY, AREA) documented on the OSM wiki for that value: `downhill` (WAY AREA), `nordic` (WAY), `skitour` (WAY AREA), `hike` (WAY), `sled` (WAY AREA), `sleigh` (WAY AREA), `ice_skate` (WAY AREA), `snow_park` (NODE AREA), `playground` (NODE AREA), `ski_jump` (WAY AREA), `fatbike` (WAY), `snowkite` (NODE AREA), `snowmobile` (WAY), `halfpipe` (WAY AREA), `connection` (WAY AREA), `snowshoe` (WAY AREA), and the semicolon combinations (WAY).

#### Scenario: Way-only value declared as way
- **WHEN** a stylesheet consumer loads the type for `piste:type=nordic`
- **THEN** the type is declared for WAY objects only

#### Scenario: Node-and-area value declared for both
- **WHEN** a stylesheet consumer loads the type for `piste:type=snow_park`
- **THEN** the type is declared for both NODE and AREA objects

#### Scenario: Way-and-area value declared for both
- **WHEN** a stylesheet consumer loads the type for `piste:type=skitour`
- **THEN** the type is declared for both WAY and AREA objects

### Requirement: Style definitions for new piste types
Each new piste type SHALL have a corresponding style definition in `include/piste.oss` where visualization is obvious, reusing existing type/style definitions for similar types (e.g. `piste_downhill_easy`) as templates. Downhill difficulty variants SHALL use the standard piste difficulty colors (green for novice, blue for easy, red for intermediate, black for advanced, orange for expert, yellow for freeride, near-black for extreme). Other types SHALL use the color scheme of OpenSnowMap where available.

#### Scenario: Downhill difficulty has a style
- **WHEN** a map is rendered with a `piste:type=downhill` + `piste:difficulty=novice` way
- **THEN** the way is drawn in green following the existing `piste_downhill_easy` style template

#### Scenario: Nordic trail has a style
- **WHEN** a map is rendered with a `piste:type=nordic` way
- **THEN** the way is drawn with a style derived from the existing piste way styles

#### Scenario: Node type has a style
- **WHEN** a map is rendered with a `piste:type=snow_park` node
- **THEN** the node is drawn with a dedicated icon symbol

#### Scenario: Area type has a style
- **WHEN** a map is rendered with a `piste:type=sled` area
- **THEN** the area is drawn with a fill and border derived from the existing `piste_downhill_*` area styles

### Requirement: New piste types participate in rendering priority groups
The new piste types SHALL be added to the piste GROUP priority definition in `winter-sports.oss` so they render at the same priority as the existing piste types.

#### Scenario: New types share priority with existing piste types
- **WHEN** a stylesheet consumer loads the GROUP definitions of `winter-sports.oss`
- **THEN** all new piste types are in the same GROUP as `piste_downhill_easy`, `piste_downhill_intermediate`, and `piste_downhill_advanced`

### Requirement: Existing piste types and styles remain unchanged
The existing type definitions for `piste_downhill_easy`, `piste_downhill_intermediate`, and `piste_downhill_advanced` SHALL keep their current names and behavior; the change SHALL only add new definitions.

#### Scenario: Existing types still resolve
- **WHEN** a stylesheet consumer loads the type for `piste:type=downhill` + `piste:difficulty=easy`
- **THEN** the existing `piste_downhill_easy` type definition is still present and unchanged
