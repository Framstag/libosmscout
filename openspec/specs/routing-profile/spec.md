# routing-profile

Routing profile configuration for multi-vehicle routing.

## Purpose

Provide a `RoutingProfile` configuration object that specifies vehicle type (car, bicycle, pedestrian) and optional restrictions (avoid tolls, ferries, unpaved roads). Used by both route calculation and live navigation.

## Requirements

### Requirement: RoutingProfile Java data class

A new Java class `RoutingProfile` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with fields: `Vehicle vehicle` (default `CAR`), `boolean avoidTolls` (default false), `boolean avoidFerries` (default false), `boolean avoidUnpaved` (default false). Multiple constructors SHALL be provided: no-arg (defaults), vehicle-only, and full.

#### Scenario: Create default profile
- **WHEN** `new RoutingProfile()` is constructed
- **THEN** `vehicle` is `CAR`, `avoidTolls` is false, `avoidFerries` is false, `avoidUnpaved` is false

#### Scenario: Create bicycle profile
- **WHEN** `new RoutingProfile(Vehicle.BICYCLE)` is constructed
- **THEN** `vehicle` is `BICYCLE` and all avoid flags are false

#### Scenario: Create pedestrian profile with avoid flags
- **WHEN** `new RoutingProfile(Vehicle.PEDESTRIAN, true, false, true)` is constructed
- **THEN** `vehicle` is `PEDESTRIAN`, `avoidTolls` is true, `avoidFerries` is false, `avoidUnpaved` is true

### Requirement: Vehicle Java enum

A new Java enum `Vehicle` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with values `CAR`, `BICYCLE`, `PEDESTRIAN`.

#### Scenario: Enum values exist
- **WHEN** `Vehicle.valueOf("CAR")`, `Vehicle.valueOf("BICYCLE")`, `Vehicle.valueOf("PEDESTRIAN")` are called
- **THEN** each returns the corresponding enum value

### Requirement: RoutingProfile bridged through JNI

The JNI implementation of `calculateRouteAsync` SHALL accept a `RoutingProfile` parameter, extract its fields, and pass them to the C++ routing service. The vehicle type SHALL select the appropriate speed map and routing profile (car/bicycle/pedestrian). Avoid flags SHALL be passed to the routing parameter.

#### Scenario: Car profile uses car speed map
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.CAR)`
- **THEN** the JNI layer creates a `FastestPathRoutingProfile` with the car speed map (50 km/h avg, 120 km/h max)

#### Scenario: Bicycle profile uses bicycle speed map
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.BICYCLE)`
- **THEN** the JNI layer creates a `FastestPathRoutingProfile` with the bicycle speed map (15 km/h avg, 30 km/h max)

#### Scenario: Pedestrian profile uses pedestrian speed map
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.PEDESTRIAN)`
- **THEN** the JNI layer creates a `FastestPathRoutingProfile` with the pedestrian speed map (5 km/h avg, 10 km/h max)

#### Scenario: Avoid tolls flag passed to routing
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.CAR, true, false, false)`
- **THEN** the routing parameter SHALL include a cost penalty for toll roads

#### Scenario: Avoid ferries flag passed to routing
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.CAR, false, true, false)`
- **THEN** the routing parameter SHALL include a cost penalty for ferry routes

### Requirement: Vehicle type passed to startNavigation

`OSMScoutClient.startNavigation` SHALL accept a `Vehicle` parameter. The `JavaNavigationController` SHALL use this vehicle type when constructing the `NavigationEngine` and `DataAgent`.

#### Scenario: Start navigation with bicycle
- **WHEN** `startNavigation(routeHandle, Vehicle.BICYCLE, listener)` is called
- **THEN** the `JavaNavigationController` is initialized with `osmscout::vehicleBicycle`
- **AND** the `DataAgent` filters for bicycle-routable ways

#### Scenario: Start navigation with pedestrian
- **WHEN** `startNavigation(routeHandle, Vehicle.PEDESTRIAN, listener)` is called
- **THEN** the `JavaNavigationController` is initialized with `osmscout::vehiclePedestrian`
- **AND** the `DataAgent` filters for pedestrian-routable ways

### Requirement: Backward compatibility

The existing `calculateRouteAsync(double, double, double, double, RouteCallback)` overload SHALL continue to work and SHALL default to `Vehicle.CAR`. The existing `startNavigation(long, NavigationListener)` overload SHALL continue to work and SHALL default to `Vehicle.CAR`.

#### Scenario: Existing calculateRouteAsync still works
- **WHEN** `calculateRouteAsync(startLat, startLon, destLat, destLon, callback)` is called without a profile
- **THEN** the route is calculated with the car profile

#### Scenario: Existing startNavigation still works
- **WHEN** `startNavigation(routeHandle, listener)` is called without a vehicle
- **THEN** the navigation engine is initialized with `vehicleCar`
