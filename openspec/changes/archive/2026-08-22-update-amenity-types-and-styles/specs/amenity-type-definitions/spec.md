# amenity-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for `amenity=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig`, are searchable (POI/address), and are renderable. Element types (node/way/area) and features follow the OSM wiki [Key:amenity](https://wiki.openstreetmap.org/wiki/Key:amenity) element table and the individual tag pages. Only values with taginfo usage >= 0.01% are covered; discouraged values are excluded.

## ADDED Requirements

### Requirement: Sustenance amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` sustenance values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=biergarten` | `amenity_biergarten` | node, area |
| `amenity=food_court` | `amenity_food_court` | node, area |
| `amenity=ice_cream` | `amenity_ice_cream` | node, area |

#### Scenario: Biergarten type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_biergarten`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=biergarten` SHALL be importable as that type

#### Scenario: Food court type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_food_court`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=food_court` SHALL be importable as that type

#### Scenario: Ice cream type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_ice_cream`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=ice_cream` SHALL be importable as that type

### Requirement: Education amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` education values, with element types as specified. Building-tagged areas SHALL match a `_building` variant type (following the existing amenity pattern):

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=college` | `amenity_college` / `amenity_college_building` | node, area |
| `amenity=university` | `amenity_university` / `amenity_university_building` | node, area |
| `amenity=childcare` | `amenity_childcare` / `amenity_childcare_building` | node, area |
| `amenity=music_school` | `amenity_music_school` / `amenity_music_school_building` | node, area |
| `amenity=language_school` | `amenity_language_school` / `amenity_language_school_building` | node, area |
| `amenity=prep_school` | `amenity_prep_school` / `amenity_prep_school_building` | node, area |
| `amenity=dancing_school` | `amenity_dancing_school` | node, area |
| `amenity=driving_school` | `amenity_driving_school` / `amenity_driving_school_building` | node, area |
| `amenity=training` | `amenity_training` | node, area |
| `amenity=research_institute` | `amenity_research_institute` / `amenity_research_institute_building` | node, area |
| `amenity=dojo` | `amenity_dojo` / `amenity_dojo_building` | node, area |

#### Scenario: College type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_college` and `amenity_college_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=college` SHALL be importable as `amenity_college`
- **AND** areas tagged `amenity=college` with a `building` tag SHALL be importable as `amenity_college_building`

#### Scenario: University type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_university` and `amenity_university_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=university` SHALL be importable as `amenity_university`

#### Scenario: Childcare type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_childcare` and `amenity_childcare_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=childcare` SHALL be importable as `amenity_childcare`

#### Scenario: Music school type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_music_school` and `amenity_music_school_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=music_school` SHALL be importable as `amenity_music_school`

#### Scenario: Language school type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_language_school` and `amenity_language_school_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=language_school` SHALL be importable as `amenity_language_school`

#### Scenario: Prep school type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_prep_school` and `amenity_prep_school_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=prep_school` SHALL be importable as `amenity_prep_school`

#### Scenario: Dancing school type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_dancing_school`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=dancing_school` SHALL be importable as that type

#### Scenario: Driving school type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_driving_school` and `amenity_driving_school_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=driving_school` SHALL be importable as `amenity_driving_school`

#### Scenario: Training type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_training`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=training` SHALL be importable as that type

#### Scenario: Research institute type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_research_institute` and `amenity_research_institute_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=research_institute` SHALL be importable as `amenity_research_institute`

#### Scenario: Dojo type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_dojo` and `amenity_dojo_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=dojo` SHALL be importable as `amenity_dojo`

### Requirement: Transportation amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` transportation values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=bicycle_rental` | `amenity_bicycle_rental` | node, area |
| `amenity=bicycle_repair_station` | `amenity_bicycle_repair_station` | node, area |
| `amenity=motorcycle_parking` | `amenity_motorcycle_parking` | node, area |
| `amenity=car_rental` | `amenity_car_rental` / `amenity_car_rental_building` | node, area |
| `amenity=car_sharing` | `amenity_car_sharing` | node, area |
| `amenity=car_wash` | `amenity_car_wash` | node, area |
| `amenity=vehicle_inspection` | `amenity_vehicle_inspection` | node, area |
| `amenity=weighbridge` | `amenity_weighbridge` | node, area |
| `amenity=parcel_locker` | `amenity_parcel_locker` | node, area |
| `amenity=boat_rental` | `amenity_boat_rental` | node, area |
| `amenity=boat_storage` | `amenity_boat_storage` | node, area |
| `amenity=public_bookcase` | `amenity_public_bookcase` | node, area |
| `amenity=bbq` | `amenity_bbq` | node |
| `amenity=loading_dock` | `amenity_loading_dock` | node |
| `amenity=trolley_bay` | `amenity_trolley_bay` | node, area |
| `amenity=ticket_validator` | `amenity_ticket_validator` | node, way |
| `amenity=compressed_air` | `amenity_compressed_air` | node, area |
| `amenity=vacuum_cleaner` | `amenity_vacuum_cleaner` | node, area |

#### Scenario: Bicycle rental type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_bicycle_rental`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=bicycle_rental` SHALL be importable as that type

#### Scenario: Bicycle repair station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_bicycle_repair_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=bicycle_repair_station` SHALL be importable as that type

#### Scenario: Motorcycle parking type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_motorcycle_parking`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=motorcycle_parking` SHALL be importable as that type

#### Scenario: Car rental type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_car_rental` and `amenity_car_rental_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=car_rental` SHALL be importable as `amenity_car_rental`

#### Scenario: Car sharing type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_car_sharing`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=car_sharing` SHALL be importable as that type

#### Scenario: Car wash type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_car_wash`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=car_wash` SHALL be importable as that type

#### Scenario: Vehicle inspection type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_vehicle_inspection`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=vehicle_inspection` SHALL be importable as that type

#### Scenario: Weighbridge type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_weighbridge`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=weighbridge` SHALL be importable as that type

#### Scenario: Parcel locker type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_parcel_locker`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=parcel_locker` SHALL be importable as that type

#### Scenario: Boat rental type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_boat_rental`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=boat_rental` SHALL be importable as that type

#### Scenario: Boat storage type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_boat_storage`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=boat_storage` SHALL be importable as that type

#### Scenario: Public bookcase type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_public_bookcase`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=public_bookcase` SHALL be importable as that type

#### Scenario: BBQ type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_bbq`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=bbq` SHALL be importable as that type

#### Scenario: Loading dock type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_loading_dock`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=loading_dock` SHALL be importable as that type

#### Scenario: Trolley bay type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_trolley_bay`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=trolley_bay` SHALL be importable as that type

#### Scenario: Ticket validator type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_ticket_validator`
- **THEN** the type SHALL exist
- **AND** nodes and ways tagged `amenity=ticket_validator` SHALL be importable as that type

### Requirement: Financial amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` financial values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=bureau_de_change` | `amenity_bureau_de_change` / `amenity_bureau_de_change_building` | node, area |
| `amenity=money_transfer` | `amenity_money_transfer` | node, area |
| `amenity=mobile_money_agent` | `amenity_mobile_money_agent` | node, area |
| `amenity=payment_terminal` | `amenity_payment_terminal` | node |

#### Scenario: Bureau de change type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_bureau_de_change` and `amenity_bureau_de_change_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=bureau_de_change` SHALL be importable as `amenity_bureau_de_change`

#### Scenario: Money transfer type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_money_transfer`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=money_transfer` SHALL be importable as that type

#### Scenario: Mobile money agent type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_mobile_money_agent`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=mobile_money_agent` SHALL be importable as that type

#### Scenario: Payment terminal type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_payment_terminal`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=payment_terminal` SHALL be importable as that type

### Requirement: Healthcare amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` healthcare values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=clinic` | `amenity_clinic` / `amenity_clinic_building` | node, area |
| `amenity=dentist` | `amenity_dentist` / `amenity_dentist_building` | node, area |
| `amenity=veterinary` | `amenity_veterinary` / `amenity_veterinary_building` | node, area |
| `amenity=health_post` | `amenity_health_post` | node, area |
| `amenity=social_facility` | `amenity_social_facility` / `amenity_social_facility_building` | node, area |

#### Scenario: Clinic type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_clinic` and `amenity_clinic_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=clinic` SHALL be importable as `amenity_clinic`

#### Scenario: Dentist type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_dentist` and `amenity_dentist_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=dentist` SHALL be importable as `amenity_dentist`

#### Scenario: Veterinary type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_veterinary` and `amenity_veterinary_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=veterinary` SHALL be importable as `amenity_veterinary`

#### Scenario: Health post type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_health_post`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=health_post` SHALL be importable as that type

#### Scenario: Social facility type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_social_facility` and `amenity_social_facility_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=social_facility` SHALL be importable as `amenity_social_facility`

### Requirement: Entertainment and culture amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` entertainment, arts and culture values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=arts_centre` | `amenity_arts_centre` / `amenity_arts_centre_building` | node, area |
| `amenity=cinema` | `amenity_cinema` / `amenity_cinema_building` | node, area |
| `amenity=theatre` | `amenity_theatre` / `amenity_theatre_building` | node, area |
| `amenity=community_centre` | `amenity_community_centre` / `amenity_community_centre_building` | node, area |
| `amenity=conference_centre` | `amenity_conference_centre` / `amenity_conference_centre_building` | node, area |
| `amenity=events_venue` | `amenity_events_venue` / `amenity_events_venue_building` | node, area |
| `amenity=nightclub` | `amenity_nightclub` / `amenity_nightclub_building` | node, area |
| `amenity=casino` | `amenity_casino` / `amenity_casino_building` | node, area |
| `amenity=gambling` | `amenity_gambling` | node, area |
| `amenity=social_centre` | `amenity_social_centre` / `amenity_social_centre_building` | node, area |
| `amenity=studio` | `amenity_studio` | node, area |
| `amenity=fountain` | `amenity_fountain` | node, area |
| `amenity=marketplace` | `amenity_marketplace` | node, area |

#### Scenario: Arts centre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_arts_centre` and `amenity_arts_centre_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=arts_centre` SHALL be importable as `amenity_arts_centre`

#### Scenario: Cinema type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_cinema` and `amenity_cinema_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=cinema` SHALL be importable as `amenity_cinema`

#### Scenario: Theatre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_theatre` and `amenity_theatre_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=theatre` SHALL be importable as `amenity_theatre`

#### Scenario: Community centre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_community_centre` and `amenity_community_centre_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=community_centre` SHALL be importable as `amenity_community_centre`

#### Scenario: Conference centre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_conference_centre` and `amenity_conference_centre_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=conference_centre` SHALL be importable as `amenity_conference_centre`

#### Scenario: Events venue type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_events_venue` and `amenity_events_venue_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=events_venue` SHALL be importable as `amenity_events_venue`

#### Scenario: Nightclub type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_nightclub` and `amenity_nightclub_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=nightclub` SHALL be importable as `amenity_nightclub`

#### Scenario: Casino type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_casino` and `amenity_casino_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=casino` SHALL be importable as `amenity_casino`

#### Scenario: Gambling type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_gambling`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=gambling` SHALL be importable as that type

#### Scenario: Social centre type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_social_centre` and `amenity_social_centre_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=social_centre` SHALL be importable as `amenity_social_centre`

#### Scenario: Studio type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_studio`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=studio` SHALL be importable as that type

#### Scenario: Fountain type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_fountain`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=fountain` SHALL be importable as that type

#### Scenario: Marketplace type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_marketplace`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=marketplace` SHALL be importable as that type

### Requirement: Public service amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` public service values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=townhall` | `amenity_townhall` / `amenity_townhall_building` | node, area |
| `amenity=courthouse` | `amenity_courthouse` / `amenity_courthouse_building` | node, area |
| `amenity=fire_station` | `amenity_fire_station` / `amenity_fire_station_building` | node, area |
| `amenity=prison` | `amenity_prison` / `amenity_prison_building` | node, area |
| `amenity=ranger_station` | `amenity_ranger_station` / `amenity_ranger_station_building` | node, area |
| `amenity=post_depot` | `amenity_post_depot` / `amenity_post_depot_building` | node, area |
| `amenity=crematorium` | `amenity_crematorium` / `amenity_crematorium_building` | node, area |
| `amenity=funeral_hall` | `amenity_funeral_hall` / `amenity_funeral_hall_building` | node, area |
| `amenity=monastery` | `amenity_monastery` / `amenity_monastery_building` | node, area |
| `amenity=public_bath` | `amenity_public_bath` / `amenity_public_bath_building` | node, area |

#### Scenario: Townhall type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_townhall` and `amenity_townhall_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=townhall` SHALL be importable as `amenity_townhall`

#### Scenario: Courthouse type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_courthouse` and `amenity_courthouse_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=courthouse` SHALL be importable as `amenity_courthouse`

#### Scenario: Fire station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_fire_station` and `amenity_fire_station_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=fire_station` SHALL be importable as `amenity_fire_station`

#### Scenario: Prison type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_prison` and `amenity_prison_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=prison` SHALL be importable as `amenity_prison`

#### Scenario: Ranger station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_ranger_station` and `amenity_ranger_station_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=ranger_station` SHALL be importable as `amenity_ranger_station`

#### Scenario: Post depot type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_post_depot` and `amenity_post_depot_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=post_depot` SHALL be importable as `amenity_post_depot`

#### Scenario: Crematorium type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_crematorium` and `amenity_crematorium_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=crematorium` SHALL be importable as `amenity_crematorium`

#### Scenario: Funeral hall type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_funeral_hall` and `amenity_funeral_hall_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=funeral_hall` SHALL be importable as `amenity_funeral_hall`

#### Scenario: Monastery type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_monastery` and `amenity_monastery_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=monastery` SHALL be importable as `amenity_monastery`

#### Scenario: Public bath type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_public_bath` and `amenity_public_bath_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=public_bath` SHALL be importable as `amenity_public_bath`

### Requirement: Facility amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` facility values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=telephone` | `amenity_telephone` | node |
| `amenity=shower` | `amenity_shower` | node, area |
| `amenity=letter_box` | `amenity_letter_box` | node |
| `amenity=lounger` | `amenity_lounger` | node |
| `amenity=chair` | `amenity_chair` | node |
| `amenity=table` | `amenity_table` | node |
| `amenity=dressing_room` | `amenity_dressing_room` | node, area |
| `amenity=smoking_area` | `amenity_smoking_area` | node, area |
| `amenity=reception_desk` | `amenity_reception_desk` | node, area |
| `amenity=sanitary_dump_station` | `amenity_sanitary_dump_station` | node, area |

#### Scenario: Telephone type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_telephone`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=telephone` SHALL be importable as that type

#### Scenario: Shower type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_shower`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=shower` SHALL be importable as that type

#### Scenario: Letter box type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_letter_box`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=letter_box` SHALL be importable as that type

#### Scenario: Lounger type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_lounger`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=lounger` SHALL be importable as that type

#### Scenario: Chair type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_chair`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=chair` SHALL be importable as that type

#### Scenario: Table type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_table`
- **THEN** the type SHALL exist
- **AND** nodes tagged `amenity=table` SHALL be importable as that type

#### Scenario: Dressing room type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_dressing_room`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=dressing_room` SHALL be importable as that type

#### Scenario: Smoking area type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_smoking_area`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=smoking_area` SHALL be importable as that type

#### Scenario: Reception desk type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_reception_desk`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=reception_desk` SHALL be importable as that type

#### Scenario: Sanitary dump station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_sanitary_dump_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=sanitary_dump_station` SHALL be importable as that type

### Requirement: Waste management amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` waste management values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=waste_transfer_station` | `amenity_waste_transfer_station` | node, area |
| `amenity=waste_dump_site` | `amenity_waste_dump_site` | node, area |

#### Scenario: Waste transfer station type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_waste_transfer_station`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=waste_transfer_station` SHALL be importable as that type

#### Scenario: Waste dump site type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_waste_dump_site`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=waste_dump_site` SHALL be importable as that type

### Requirement: Animal and outdoor amenity types

The import-time stylesheet SHALL define feature types for the following `amenity=*` animal/outdoor values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=animal_shelter` | `amenity_animal_shelter` | node, area |
| `amenity=animal_boarding` | `amenity_animal_boarding` | node, area |
| `amenity=animal_breeding` | `amenity_animal_breeding` | node, area |
| `amenity=feeding_place` | `amenity_feeding_place` | node, area |
| `amenity=game_feeding` | `amenity_game_feeding` | node, area |
| `amenity=hunting_stand` | `amenity_hunting_stand` | node, area |

#### Scenario: Animal shelter type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_animal_shelter`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=animal_shelter` SHALL be importable as that type

#### Scenario: Animal boarding type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_animal_boarding`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=animal_boarding` SHALL be importable as that type

#### Scenario: Animal breeding type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_animal_breeding`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=animal_breeding` SHALL be importable as that type

#### Scenario: Feeding place type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_feeding_place`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=feeding_place` SHALL be importable as that type

#### Scenario: Game feeding type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_game_feeding`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=game_feeding` SHALL be importable as that type

#### Scenario: Hunting stand type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_hunting_stand`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=hunting_stand` SHALL be importable as that type

### Requirement: Other amenity types

The import-time stylesheet SHALL define feature types for the following remaining `amenity=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `amenity=internet_cafe` | `amenity_internet_cafe` / `amenity_internet_cafe_building` | node, area |
| `amenity=driver_training` | `amenity_driver_training` | node, area |
| `amenity=lavoir` | `amenity_lavoir` | node, area |
| `amenity=love_hotel` | `amenity_love_hotel` | node, area |

#### Scenario: Internet cafe type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_internet_cafe` and `amenity_internet_cafe_building`
- **THEN** both types SHALL exist
- **AND** nodes and areas tagged `amenity=internet_cafe` SHALL be importable as `amenity_internet_cafe`

#### Scenario: Driver training type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_driver_training`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=driver_training` SHALL be importable as that type

#### Scenario: Lavoir type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_lavoir`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=lavoir` SHALL be importable as that type

#### Scenario: Love hotel type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_love_hotel`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `amenity=love_hotel` SHALL be importable as that type

### Requirement: Existing amenity types extended with wiki element types

The import-time stylesheet SHALL extend the element types of the following existing types to match the OSM wiki:

| Type name | Previously | Now |
|-----------|------------|-----|
| `amenity_bench` | node | node, way |
| `amenity_toilets` | node | node, area |
| `amenity_shelter` | node | node, area |

#### Scenario: Bench type accepts ways
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_bench`
- **THEN** the type SHALL exist
- **AND** ways tagged `amenity=bench` SHALL be importable as that type

#### Scenario: Toilets type accepts areas
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_toilets`
- **THEN** the type SHALL exist
- **AND** areas tagged `amenity=toilets` SHALL be importable as that type

#### Scenario: Shelter type accepts areas
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_shelter`
- **THEN** the type SHALL exist
- **AND** areas tagged `amenity=shelter` SHALL be importable as that type

### Requirement: Discouraged and non-relevant values excluded

The import-time stylesheet SHALL NOT define dedicated feature types for `amenity` values that are discouraged on the OSM wiki or have taginfo usage below 0.01%.

#### Scenario: Discouraged public building value is not typed
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_public_building`
- **THEN** no such type SHALL exist
- **AND** objects tagged `amenity=public_building` SHALL fall back to the generic `amenity`/`amenity_building` types

#### Scenario: Discouraged nursing home value is not typed
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `amenity_nursing_home`
- **THEN** no such type SHALL exist
- **AND** objects tagged `amenity=nursing_home` SHALL fall back to the generic `amenity`/`amenity_building` types

### Requirement: Rendering rules for new amenity types

The rendering stylesheets SHALL define rendering rules for the newly added amenity types, so they are visible on maps.

#### Scenario: New amenity building types are rendered
- **GIVEN** a rendering stylesheet that includes the amenity rendering module
- **WHEN** a map is rendered containing building-tagged areas of the new `_building` amenity types
- **THEN** the areas SHALL be drawn with a fill color and border

#### Scenario: Major new amenity area types are rendered with distinctive colors
- **GIVEN** a rendering stylesheet that includes the amenity rendering module
- **WHEN** a map is rendered containing areas of the new amenity types (`amenity_fire_station`, `amenity_clinic`, `amenity_dentist`, `amenity_veterinary`, `amenity_cinema`, `amenity_theatre`, `amenity_townhall`, `amenity_courthouse`, `amenity_community_centre`, `amenity_marketplace`, `amenity_ice_cream`, `amenity_biergarten`, `amenity_car_wash`, `amenity_fountain`, `amenity_hunting_stand`, `amenity_university`, `amenity_college`)
- **THEN** the areas SHALL be drawn with their category-specific fill color

#### Scenario: New amenity node types are rendered
- **GIVEN** a rendering stylesheet that includes the amenity rendering module
- **WHEN** a map is rendered containing nodes of the new amenity types
- **THEN** the nodes SHALL be drawn with a symbol icon or a text label
- **AND** iconic types (`amenity_fire_station`, `amenity_telephone`, `amenity_fountain`, `amenity_cinema`, `amenity_theatre`, `amenity_townhall`, `amenity_marketplace`, `amenity_prison`, `amenity_bbq`, `amenity_hunting_stand`, `amenity_ice_cream`, `amenity_car_wash`, `amenity_parcel_locker`, `amenity_shower`, `amenity_public_bath`, `amenity_university`, `amenity_motorcycle_parking`, `amenity_nightclub`, `amenity_community_centre`, `amenity_veterinary`, `amenity_boat_rental`, `amenity_biergarten`) SHALL be drawn with a dedicated symbol

#### Scenario: New amenity types carry name labels
- **GIVEN** a rendering stylesheet that includes the amenity rendering module
- **WHEN** a map is rendered containing named nodes or areas of the new amenity types
- **THEN** the objects SHALL be labelled with their `name`
