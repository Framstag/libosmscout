# routing-profile

Routing profile configuration for multi-vehicle routing.

## Purpose

Provide a `RoutingProfile` configuration object that specifies vehicle type (car, bicycle, pedestrian) and optional restrictions (avoid tolls, ferries, unpaved roads). Used by both route calculation and live navigation.

## Requirements

### Requirement: RoutingProfile Java data class

A new Java class `RoutingProfile` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with fields: `Vehicle vehicle` (default `CAR`), `boolean avoidTolls` (default false), `boolean avoidFerries` (default false), `boolean avoidUnpaved` (default false), `double maxSpeedKmh` (default `0`, meaning the vehicle's built-in profile).

#### Scenario: The constructor set is complete
- **WHEN** the constructors of `RoutingProfile` are inspected
- **THEN** it offers a no-arg one, a vehicle-only one, a vehicle-plus-speed one, a vehicle-plus-avoid-flags one, and a vehicle-plus-avoid-flags-plus-speed one

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

### Requirement: Routing profile maximum speed

`RoutingProfile` SHALL carry the vehicle's usual maximum speed in km/h as `double maxSpeedKmh`. The value `0` SHALL mean "use the routing engine's built-in profile for the vehicle" (car 120, bicycle 30, pedestrian 10 km/h), and every constructor that does not name a speed SHALL set the field to `0`.

#### Scenario: Default profile requests the built-in speed
- **WHEN** `new RoutingProfile()` is constructed
- **THEN** `maxSpeedKmh` is `0`

#### Scenario: Vehicle-only profile requests the built-in speed
- **WHEN** `new RoutingProfile(Vehicle.BICYCLE)` is constructed
- **THEN** `maxSpeedKmh` is `0`
- **AND** the vehicle is `BICYCLE`

#### Scenario: Vehicle and speed constructor carries the speed
- **WHEN** `new RoutingProfile(Vehicle.BICYCLE, 20.0)` is constructed
- **THEN** the vehicle is `BICYCLE`, `maxSpeedKmh` is `20.0` and all avoid flags are false

#### Scenario: Full constructor carries the speed
- **WHEN** `new RoutingProfile(Vehicle.PEDESTRIAN, true, false, true, 4.0)` is constructed
- **THEN** `maxSpeedKmh` is `4.0` and the avoid flags are `true`, `false`, `true`

#### Scenario: The avoid-flag constructor without a speed keeps meaning the built-in profile
- **WHEN** `new RoutingProfile(Vehicle.CAR, true, true, true)` is constructed
- **THEN** `maxSpeedKmh` is `0`

### Requirement: RoutingProfile bridged through JNI

The JNI implementation of `calculateRouteAsync` SHALL accept a `RoutingProfile` parameter, extract its fields, and pass them to the C++ routing service. The vehicle type SHALL select the appropriate speed map and routing profile (car/bicycle/pedestrian). Avoid flags SHALL be passed to the routing parameter. A usable maximum speed SHALL parametrize the profile; an unusable one SHALL select the vehicle's built-in default instead of failing the calculation.

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

#### Scenario: Car maximum speed is a ceiling
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.CAR, 90.0)`
- **THEN** the car profile's maximum speed is 90 km/h
- **AND** the per-road-type speed map is unchanged, so a road type faster than 90 km/h is capped at 90

#### Scenario: Bicycle maximum speed is the flat speed
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.BICYCLE, 20.0)`
- **THEN** every bicycle-routable way type is parametrized with 20 km/h

#### Scenario: Pedestrian maximum speed is the flat speed
- **WHEN** `calculateRouteAsync` is called with `RoutingProfile(Vehicle.PEDESTRIAN, 4.0)`
- **THEN** every foot-routable way type is parametrized with 4 km/h

#### Scenario: An unusable maximum speed selects the built-in default
- **WHEN** `calculateRouteAsync` is called with a profile whose `maxSpeedKmh` is `0`, non-finite, below 1 or above 300
- **THEN** the profile is parametrized with the built-in default of its vehicle (car 120, bicycle 30, pedestrian 10 km/h)
- **AND** the calculation does not fail and reports no error

#### Scenario: A profile without the field selects the built-in default
- **WHEN** the passed profile class does not declare `maxSpeedKmh` (an older caller)
- **THEN** the field lookup is handled, no exception is left pending, and the built-in default is used

#### Scenario: The applied speed is observable
- **WHEN** a profile is created
- **THEN** the log line reporting the created profile names the applied maximum speed in km/h

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
