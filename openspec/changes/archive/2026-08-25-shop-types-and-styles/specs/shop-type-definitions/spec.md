# shop-type-definitions Specification

## Purpose

Defines the import-time OSM feature types for `shop=*` values missing from `stylesheets/map.ost`, so these features exist in the database `TypeConfig` and are renderable. Element types follow the OSM wiki [Key:shop](https://wiki.openstreetmap.org/wiki/Key:shop) element table: shop values are used on nodes and areas, not on ways. Value selection follows the wiki's documented values plus taginfo usage >= 0.02% for values not discouraged.

## ADDED Requirements

### Requirement: Food and beverage shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` food/beverage values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=brewing_supplies` | `shop_brewing_supplies` | node, area |
| `shop=nuts` | `shop_nuts` | node, area |
| `shop=pasta` | `shop_pasta` | node, area |
| `shop=tortilla` | `shop_tortilla` | node, area |
| `shop=water` | `shop_water` | node, area |

Each type SHALL have a `_building` variant (`shop_brewing_supplies_building`, etc.) restricted to areas carrying a `building` tag, following the existing food/beverage pattern.

#### Scenario: Nuts shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_nuts`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=nuts` SHALL be importable as that type

#### Scenario: Water shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_water`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=water` SHALL be importable as that type

### Requirement: General store, department store and mall shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` general store values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=department_store` | `shop_department_store` | node, area |
| `shop=general` | `shop_general` | node, area |
| `shop=kiosk` | `shop_kiosk` | node, area |
| `shop=mall` | `shop_mall` | node, area |
| `shop=wholesale` | `shop_wholesale` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Mall shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_mall`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=mall` SHALL be importable as that type

#### Scenario: Kiosk shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_kiosk`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=kiosk` SHALL be importable as that type

### Requirement: Clothing, shoes and accessories shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` clothing values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=baby_goods` | `shop_baby_goods` | node, area |
| `shop=bag` | `shop_bag` | node, area |
| `shop=clothes` | `shop_clothes` | node, area |
| `shop=fabric` | `shop_fabric` | node, area |
| `shop=fashion_accessories` | `shop_fashion_accessories` | node, area |
| `shop=jewelry` | `shop_jewelry` | node, area |
| `shop=leather` | `shop_leather` | node, area |
| `shop=sewing` | `shop_sewing` | node, area |
| `shop=shoes` | `shop_shoes` | node, area |
| `shop=shoe_repair` | `shop_shoe_repair` | node, area |
| `shop=tailor` | `shop_tailor` | node, area |
| `shop=watches` | `shop_watches` | node, area |
| `shop=wool` | `shop_wool` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Clothes shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_clothes`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=clothes` SHALL be importable as that type

#### Scenario: Shoes shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_shoes`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=shoes` SHALL be importable as that type

### Requirement: Discount store and charity shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` discount/charity values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=charity` | `shop_charity` | node, area |
| `shop=second_hand` | `shop_second_hand` | node, area |
| `shop=variety_store` | `shop_variety_store` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Charity shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_charity`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=charity` SHALL be importable as that type

#### Scenario: Variety store shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_variety_store`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=variety_store` SHALL be importable as that type

### Requirement: Health and beauty shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` health/beauty values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=beauty` | `shop_beauty` | node, area |
| `shop=chemist` | `shop_chemist` | node, area |
| `shop=cosmetics` | `shop_cosmetics` | node, area |
| `shop=erotic` | `shop_erotic` | node, area |
| `shop=hairdresser` | `shop_hairdresser` | node, area |
| `shop=hairdresser_supply` | `shop_hairdresser_supply` | node, area |
| `shop=hearing_aids` | `shop_hearing_aids` | node, area |
| `shop=herbalist` | `shop_herbalist` | node, area |
| `shop=massage` | `shop_massage` | node, area |
| `shop=medical_supply` | `shop_medical_supply` | node, area |
| `shop=nutrition_supplements` | `shop_nutrition_supplements` | node, area |
| `shop=optician` | `shop_optician` | node, area |
| `shop=perfumery` | `shop_perfumery` | node, area |
| `shop=piercing` | `shop_piercing` | node, area |
| `shop=tattoo` | `shop_tattoo` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Hairdresser shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_hairdresser`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=hairdresser` SHALL be importable as that type

#### Scenario: Optician shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_optician`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=optician` SHALL be importable as that type

### Requirement: Do-it-yourself, household, building materials and gardening shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` DIY/household values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=agrarian` | `shop_agrarian` | node, area |
| `shop=appliance` | `shop_appliance` | node, area |
| `shop=bathroom_furnishing` | `shop_bathroom_furnishing` | node, area |
| `shop=country_store` | `shop_country_store` | node, area |
| `shop=doityourself` | `shop_doityourself` | node, area |
| `shop=electrical` | `shop_electrical` | node, area |
| `shop=energy` | `shop_energy` | node, area |
| `shop=fireplace` | `shop_fireplace` | node, area |
| `shop=florist` | `shop_florist` | node, area |
| `shop=garden_centre` | `shop_garden_centre` | node, area |
| `shop=garden_furniture` | `shop_garden_furniture` | node, area |
| `shop=gas` | `shop_gas` | node, area |
| `shop=glaziery` | `shop_glaziery` | node, area |
| `shop=groundskeeping` | `shop_groundskeeping` | node, area |
| `shop=hardware` | `shop_hardware` | node, area |
| `shop=houseware` | `shop_houseware` | node, area |
| `shop=locksmith` | `shop_locksmith` | node, area |
| `shop=paint` | `shop_paint` | node, area |
| `shop=pottery` | `shop_pottery` | node, area |
| `shop=security` | `shop_security` | node, area |
| `shop=tool_hire` | `shop_tool_hire` | node, area |
| `shop=trade` | `shop_trade` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Doityourself shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_doityourself`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=doityourself` SHALL be importable as that type

#### Scenario: Florist shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_florist`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=florist` SHALL be importable as that type

### Requirement: Furniture and interior shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` furniture/interior values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=antiques` | `shop_antiques` | node, area |
| `shop=bed` | `shop_bed` | node, area |
| `shop=candles` | `shop_candles` | node, area |
| `shop=carpet` | `shop_carpet` | node, area |
| `shop=curtain` | `shop_curtain` | node, area |
| `shop=doors` | `shop_doors` | node, area |
| `shop=flooring` | `shop_flooring` | node, area |
| `shop=furniture` | `shop_furniture` | node, area |
| `shop=household_linen` | `shop_household_linen` | node, area |
| `shop=interior_decoration` | `shop_interior_decoration` | node, area |
| `shop=kitchen` | `shop_kitchen` | node, area |
| `shop=lighting` | `shop_lighting` | node, area |
| `shop=tiles` | `shop_tiles` | node, area |
| `shop=window_blind` | `shop_window_blind` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Furniture shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_furniture`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=furniture` SHALL be importable as that type

#### Scenario: Kitchen shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_kitchen`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=kitchen` SHALL be importable as that type

### Requirement: Electronics shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` electronics values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=computer` | `shop_computer` | node, area |
| `shop=electronics` | `shop_electronics` | node, area |
| `shop=hifi` | `shop_hifi` | node, area |
| `shop=mobile_phone` | `shop_mobile_phone` | node, area |
| `shop=printer_ink` | `shop_printer_ink` | node, area |
| `shop=radiotechnics` | `shop_radiotechnics` | node, area |
| `shop=telecommunication` | `shop_telecommunication` | node, area |
| `shop=vacuum_cleaner` | `shop_vacuum_cleaner` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Electronics shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_electronics`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=electronics` SHALL be importable as that type

#### Scenario: Mobile phone shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_mobile_phone`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=mobile_phone` SHALL be importable as that type

### Requirement: Outdoors, sport and vehicle shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` outdoors/vehicle values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=atv` | `shop_atv` | node, area |
| `shop=aviation` | `shop_aviation` | node, area |
| `shop=bicycle` | `shop_bicycle` | node, area |
| `shop=boat` | `shop_boat` | node, area |
| `shop=car` | `shop_car` | node, area |
| `shop=car_parts` | `shop_car_parts` | node, area |
| `shop=car_repair` | `shop_car_repair` | node, area |
| `shop=caravan` | `shop_caravan` | node, area |
| `shop=fishing` | `shop_fishing` | node, area |
| `shop=fuel` | `shop_fuel` | node, area |
| `shop=golf` | `shop_golf` | node, area |
| `shop=hunting` | `shop_hunting` | node, area |
| `shop=military_surplus` | `shop_military_surplus` | node, area |
| `shop=motorcycle` | `shop_motorcycle` | node, area |
| `shop=motorcycle_repair` | `shop_motorcycle_repair` | node, area |
| `shop=outdoor` | `shop_outdoor` | node, area |
| `shop=scooter` | `shop_scooter` | node, area |
| `shop=scuba_diving` | `shop_scuba_diving` | node, area |
| `shop=ski` | `shop_ski` | node, area |
| `shop=snowmobile` | `shop_snowmobile` | node, area |
| `shop=sports` | `shop_sports` | node, area |
| `shop=surf` | `shop_surf` | node, area |
| `shop=swimming_pool` | `shop_swimming_pool` | node, area |
| `shop=trailer` | `shop_trailer` | node, area |
| `shop=truck` | `shop_truck` | node, area |
| `shop=tyres` | `shop_tyres` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Car repair shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_car_repair`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=car_repair` SHALL be importable as that type

#### Scenario: Bicycle shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_bicycle`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=bicycle` SHALL be importable as that type

### Requirement: Art, music and hobby shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` art/music/hobby values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=art` | `shop_art` | node, area |
| `shop=camera` | `shop_camera` | node, area |
| `shop=collector` | `shop_collector` | node, area |
| `shop=craft` | `shop_craft` | node, area |
| `shop=frame` | `shop_frame` | node, area |
| `shop=games` | `shop_games` | node, area |
| `shop=model` | `shop_model` | node, area |
| `shop=music` | `shop_music` | node, area |
| `shop=musical_instrument` | `shop_musical_instrument` | node, area |
| `shop=photo` | `shop_photo` | node, area |
| `shop=trophy` | `shop_trophy` | node, area |
| `shop=video` | `shop_video` | node, area |
| `shop=video_games` | `shop_video_games` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Music shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_music`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=music` SHALL be importable as that type

#### Scenario: Video games shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_video_games`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=video_games` SHALL be importable as that type

### Requirement: Stationery, gifts, books and newspaper shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` stationery/gift values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=anime` | `shop_anime` | node, area |
| `shop=books` | `shop_books` | node, area |
| `shop=gift` | `shop_gift` | node, area |
| `shop=lottery` | `shop_lottery` | node, area |
| `shop=newsagent` | `shop_newsagent` | node, area |
| `shop=stationery` | `shop_stationery` | node, area |
| `shop=ticket` | `shop_ticket` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Books shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_books`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=books` SHALL be importable as that type

#### Scenario: Gift shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_gift`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=gift` SHALL be importable as that type

### Requirement: Other shop types

The import-time stylesheet SHALL define feature types for the following remaining `shop=*` values, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=bookmaker` | `shop_bookmaker` | node, area |
| `shop=cannabis` | `shop_cannabis` | node, area |
| `shop=copyshop` | `shop_copyshop` | node, area |
| `shop=dry_cleaning` | `shop_dry_cleaning` | node, area |
| `shop=e-cigarette` | `shop_e-cigarette` | node, area |
| `shop=funeral_directors` | `shop_funeral_directors` | node, area |
| `shop=laundry` | `shop_laundry` | node, area |
| `shop=money_lender` | `shop_money_lender` | node, area |
| `shop=outpost` | `shop_outpost` | node, area |
| `shop=party` | `shop_party` | node, area |
| `shop=pawnbroker` | `shop_pawnbroker` | node, area |
| `shop=pest_control` | `shop_pest_control` | node, area |
| `shop=pet` | `shop_pet` | node, area |
| `shop=pet_grooming` | `shop_pet_grooming` | node, area |
| `shop=pyrotechnics` | `shop_pyrotechnics` | node, area |
| `shop=religion` | `shop_religion` | node, area |
| `shop=rental` | `shop_rental` | node, area |
| `shop=storage_rental` | `shop_storage_rental` | node, area |
| `shop=tobacco` | `shop_tobacco` | node, area |
| `shop=toys` | `shop_toys` | node, area |
| `shop=travel_agency` | `shop_travel_agency` | node, area |
| `shop=vacant` | `shop_vacant` | node, area |
| `shop=vending_machine` | `shop_vending_machine` | node, area |
| `shop=weapons` | `shop_weapons` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Pet shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_pet`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=pet` SHALL be importable as that type

#### Scenario: Laundry shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_laundry`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=laundry` SHALL be importable as that type

### Requirement: Additional taginfo shop types

The import-time stylesheet SHALL define feature types for the following `shop=*` values that are not listed on the Key:shop wiki page but have taginfo usage >= 0.02% and are not discouraged, with element types as specified:

| OSM tag | Type name | Elements |
|---------|-----------|----------|
| `shop=repair` | `shop_repair` | node, area |
| `shop=estate_agent` | `shop_estate_agent` | node, area |
| `shop=printing` | `shop_printing` | node, area |
| `shop=building_materials` | `shop_building_materials` | node, area |
| `shop=mobile_phone_accessories` | `shop_mobile_phone_accessories` | node, area |
| `shop=power_tools` | `shop_power_tools` | node, area |
| `shop=gold_buyer` | `shop_gold_buyer` | node, area |
| `shop=honey` | `shop_honey` | node, area |
| `shop=rice` | `shop_rice` | node, area |

Each type SHALL have a `_building` variant.

#### Scenario: Repair shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_repair`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=repair` SHALL be importable as that type

#### Scenario: Estate agent shop type exists in type config
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_estate_agent`
- **THEN** the type SHALL exist
- **AND** nodes and areas tagged `shop=estate_agent` SHALL be importable as that type

### Requirement: Excluded shop values

The import-time stylesheet SHALL NOT define dedicated types for the following `shop=*` values, because they are deprecated, discouraged, or not real shop types:

- `shop=boutique` — deprecated, use `shop=clothes`
- `shop=fashion` — deprecated, use `shop=clothes`
- `shop=duty_free` — documented as a tagging mistake
- `shop=factory_outlet` — documented as a tagging mistake
- `shop=outlet` — documented as a tagging mistake
- `shop=no` — discouraged, "nowadays generally not used"
- semicolon-combined values (e.g. `shop=convenience;gas`)

`shop=yes` SHALL NOT get a dedicated type: it is covered by the generic `shop` type matching `EXISTS "shop"`.

#### Scenario: Boutique value falls back to generic shop type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_boutique`
- **THEN** the type SHALL NOT exist
- **AND** objects tagged `shop=boutique` SHALL be importable as the generic `shop` type

#### Scenario: Yes value falls back to generic shop type
- **GIVEN** a database imported with the type definitions
- **WHEN** the database `TypeConfig` is queried for `shop_yes`
- **THEN** the type SHALL NOT exist
- **AND** objects tagged `shop=yes` SHALL be importable as the generic `shop` type

### Requirement: Building variants for shop types

For every granular shop type, the import-time stylesheet SHALL define a `_building` variant that matches only areas carrying a `building` tag (excluding `building=no/false/0`), following the existing food/beverage pattern. The plain type SHALL match nodes and areas without the building restriction.

#### Scenario: Building variant matches building-tagged areas
- **GIVEN** a database imported with the type definitions
- **WHEN** an area tagged `shop=clothes` and `building=yes` is imported
- **THEN** the object SHALL be importable as `shop_clothes_building`
- **AND** the plain `shop_clothes` type SHALL NOT match it

#### Scenario: Plain type matches nodes
- **GIVEN** a database imported with the type definitions
- **WHEN** a node tagged `shop=clothes` is imported
- **THEN** the object SHALL be importable as `shop_clothes`

### Requirement: Rendering rules for new shop types

The rendering stylesheets SHALL define rendering rules for all newly added shop types, so they are visible on maps. All shop types SHALL remain in the `shop` group so the existing generic shop rendering (area fill, labels, node icons) applies, and building variants SHALL remain in the `shop` and `building` groups.

#### Scenario: New shop area types are rendered
- **GIVEN** a rendering stylesheet that includes the shop rendering module
- **WHEN** a map is rendered containing areas of the new shop types
- **THEN** the areas SHALL be drawn with a fill color

#### Scenario: New shop node types are rendered
- **GIVEN** a rendering stylesheet that includes the shop rendering module
- **WHEN** a map is rendered containing nodes of the new shop types
- **THEN** the nodes SHALL be rendered with a symbol or text label
