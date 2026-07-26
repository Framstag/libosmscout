## Purpose

Allow JavaScout (and other Java clients) to register synthetic POI types at runtime so that overlay markers can be rendered through the standard map style pipeline without modifying imported `.osmscout` databases.

## Requirements

### Requirement: Java builder registers synthetic POI types at runtime
The Java `OSMScoutClientBuilder` SHALL allow callers to register one or more synthetic POI type names. These type names SHALL be stored in the builder and passed to the native `build()` implementation.

#### Scenario: Register single custom POI type
- **WHEN** `withCustomPoiType("_favorite")` is called on the builder
- **THEN** the builder SHALL store "_favorite" in its custom POI type list

#### Scenario: Register multiple custom POI types
- **WHEN** `withCustomPoiType("_favorite")`, `withCustomPoiType("_search_selected")`, `withCustomPoiType("_route_start")`, and `withCustomPoiType("_track")` are chained
- **THEN** the builder SHALL store all type names in order

### Requirement: `_track` synthetic POI type is registered in JavaScout
The system SHALL register a `_track` synthetic POI type in JavaScout so imported GPX tracks can be rendered through the standard style pipeline.

#### Scenario: MainController registers track type
- **WHEN** `MainController` initializes the `OSMScoutClient`
- **THEN** it SHALL call `withCustomPoiType("_track")` in addition to the existing custom POI types

#### Scenario: Track type is available for styling
- **GIVEN** `_track` was registered during client build
- **WHEN** a map database is opened
- **THEN** the database's `TypeConfig` SHALL contain the type `_track`

### Requirement: JNI build passes custom POI types to C++ DBThread
The C++ JNI implementation of `OSMScoutClientBuilder.build()` SHALL read the `customPoiTypes` array from the Java builder object and pass the type names as a `std::vector<std::string>` to the `osmscout::DBThread` constructor.

#### Scenario: Build with custom POI types creates DBThread with those types
- **GIVEN** a Java builder with custom POI types `["_favorite", "_search_selected"]`
- **WHEN** `build()` is invoked
- **THEN** the resulting `DBThread` SHALL have registered those synthetic types in its type config

### Requirement: DBThread registers synthetic types without database re-import
The custom POI types SHALL be registered in the empty type config at `DBThread` construction time and re-registered whenever the database list changes. No re-import of `.osmscout` databases SHALL be required.

#### Scenario: Database opened after DBThread construction knows custom types
- **GIVEN** a `DBThread` constructed with custom POI type `["_favorite"]`
- **WHEN** a map database is opened
- **THEN** the database's `TypeConfig` SHALL contain the type "_favorite"
