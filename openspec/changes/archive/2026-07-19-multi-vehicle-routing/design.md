## Context

JavaScout's live navigation stack currently hard-codes `osmscout::vehicleCar` in two places:

1. **Route calculation** (`OSMScoutClient.cpp`): `FastestPathRoutingProfile` is created with a car speed map. No vehicle parameter is exposed through JNI.
2. **Navigation engine** (`JavaNavigationController`): The `NavigationEngine` is initialized with `osmscout::vehicleCar`. The `DataAgent` uses this vehicle type to filter routable objects.

The `MultiDBRoutingService` and `NavigationEngine` both already support bicycle (`vehicleBicycle`) and pedestrian (`vehiclePedestrian`) — the gap is purely in the API surface and UI.

## Goals / Non-Goals

**Goals:**
- Expose `Vehicle` enum and `RoutingProfile` in `libosmscout-client` C++ API
- Bridge profile through JNI to Java `RoutingProfile` class
- Add vehicle selector UI to JavaScout `RoutePanel`
- Wire profile through route calculation and navigation start
- Backward compatible — existing car-only callers unchanged

**Non-Goals:**
- Public transport routing (requires GTFS data, different engine)
- Route comparison (multiple profiles side-by-side)
- Waypoint routing (separate concern)
- Profile persistence between sessions (follow-up)

## Decisions

### Decision 1: Profile as parameter struct vs separate methods

**Chosen:** Single `RoutingProfile` struct with vehicle type + avoid flags, passed as optional parameter.

**Alternatives considered:**
- Separate methods per vehicle (`calculateCarRouteAsync`, `calculateBikeRouteAsync`) — rejected: explodes method count when avoid flags are added
- String-based vehicle type — rejected: no type safety, harder to evolve

### Decision 2: Avoid flags as bitmask vs individual booleans

**Chosen:** Individual booleans in both C++ and Java. Bitmask is premature optimization for 3 flags.

### Decision 3: Vehicle type in `startNavigation`

**Chosen:** Add `vehicle` parameter to `startNavigation`. The navigation engine needs the vehicle type at construction time for the `DataAgent` to filter routable objects correctly.

**Alternatives considered:**
- Infer vehicle from the route description — rejected: `RouteDescription` doesn't carry vehicle type
- Store vehicle in `RouteEntry` — rejected: mixes routing and navigation concerns

### Decision 4: Speed map per vehicle

**Chosen:** Hard-code reasonable default speed maps for bicycle (avg 15 km/h, max 30 km/h) and pedestrian (avg 5 km/h, max 10 km/h) in the JNI layer, matching the existing car speed map pattern.

**Alternatives considered:**
- Load speed maps from style config — rejected: adds complexity, no existing mechanism for per-vehicle speed tables
- Make speed map configurable in `RoutingProfile` — rejected: scope creep, can be added later

## Data Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│              ROUTE CALCULATION WITH PROFILE                        │
└─────────────────────────────────────────────────────────────────────┘

JavaScout RoutePanel                  libosmscout-client-java          libosmscout-client
      │                                      │                              │
      │ User selects "Bicycle"               │                              │
      │ User clicks "Calculate"              │                              │
      │                                      │                              │
      │ calculateRouteAsync(                 │                              │
      │   startLat, startLon,                │                              │
      │   destLat, destLon,                  │                              │
      │   RoutingProfile(BICYCLE)) ──────────┼──▶ JNI bridge                │
      │                                      │                              │
      │                                      │  Parse RoutingProfile        │
      │                                      │  Select speed map:           │
      │                                      │    bicycle: 15 km/h avg      │
      │                                      │                              │
      │                                      │  Create RoutingProfile       │
      │                                      │  with vehicleBicycle         │
      │                                      │                              │
      │                                      │  CalculateRoute(             │
      │                                      │    start, dest,              │
      │                                      │    profile) ────────────────▶│
      │                                      │                              │
      │                                      │                              │  MultiDBRoutingService
      │                                      │                              │  uses bicycle-compatible
      │                                      │                              │  ways (cycleways, paths,
      │                                      │                              │  pedestrian zones)
      │                                      │                              │
      │                                      │◀── RouteData ───────────────│
      │                                      │                              │
      │◀── RouteEntry ──────────────────────│                              │
      │                                      │                              │
      │                                      │                              │
      │ startNavigation(                      │                              │
      │   routeHandle,                       │                              │
      │   listener,                          │                              │
      │   Vehicle.BICYCLE) ──────────────────│                              │
      │                                      │                              │
      │                                      │  JavaNavigationController    │
      │                                      │  with vehicleBicycle         │
      │                                      │  DataAgent filters for       │
      │                                      │  bicycle-routable ways       │
      │                                      │                              │
```

## Types

### C++ — `libosmscout-client`

```cpp
namespace osmscout {

enum class Vehicle {
    Car,
    Bicycle,
    Pedestrian
};

struct RoutingProfile {
    Vehicle vehicle = Vehicle::Car;
    bool avoidTolls = false;
    bool avoidFerries = false;
    bool avoidUnpaved = false;  // relevant for bicycle
};

}
```

### Java — `libosmscout-client-java`

```java
package com.framstag.libosmscout.client;

public enum Vehicle {
    CAR,
    BICYCLE,
    PEDESTRIAN
}

public class RoutingProfile {
    public final Vehicle vehicle;
    public final boolean avoidTolls;
    public final boolean avoidFerries;
    public final boolean avoidUnpaved;

    public RoutingProfile() {
        this(Vehicle.CAR, false, false, false);
    }

    public RoutingProfile(Vehicle vehicle) {
        this(vehicle, false, false, false);
    }

    public RoutingProfile(Vehicle vehicle,
                          boolean avoidTolls,
                          boolean avoidFerries,
                          boolean avoidUnpaved) {
        this.vehicle = vehicle;
        this.avoidTolls = avoidTolls;
        this.avoidFerries = avoidFerries;
        this.avoidUnpaved = avoidUnpaved;
    }
}
```

### Java — API changes

```java
// New overload in OSMScoutClient
public native void calculateRouteAsync(double startLat, double startLon,
                                       double destLat, double destLon,
                                       RouteCallback callback);

public native void calculateRouteAsync(double startLat, double startLon,
                                       double destLat, double destLon,
                                       RoutingProfile profile,
                                       RouteCallback callback);

// startNavigation gains vehicle parameter
public native NavigationController startNavigation(long routeHandle,
                                                   Vehicle vehicle,
                                                   NavigationListener listener);
```

## Speed Maps

| Vehicle   | Average speed | Max speed | Routable way types                    |
|-----------|--------------|-----------|---------------------------------------|
| Car       | 50 km/h      | 120 km/h  | highways, primary, secondary, tertiary |
| Bicycle   | 15 km/h      | 30 km/h   | cycleways, paths, residential, tertiary |
| Pedestrian| 5 km/h       | 10 km/h   | footways, paths, pedestrian zones, steps |

## Risks / Trade-offs

- **[Speed accuracy]** Default speed maps are approximations. Real-world cycling speed varies by terrain, fitness, weather. → Acceptable for v1. Future: allow user-defined average speed.
- **[Routing quality]** Bicycle routing without elevation data may produce hilly routes. → Elevation data is a separate concern; basic bike routing is still useful without it.
- **[Avoid flags]** `MultiDBRoutingService` may not support all avoid flags natively. → Implement as post-filtering on the route if engine-level support is missing. Verify during implementation.
- **[JNI surface growth]** New types and overloads increase the JNI surface. → Mitigated by keeping `RoutingProfile` simple (4 fields) and reusing existing marshaling patterns.
