## 1. C++ API — Vehicle and RoutingProfile types

*Spec: routing-profile | Design: Types*

- [x] 1.1 `Vehicle` enum already exists in `OSMScoutTypes.h` (vehicleFoot, vehicleBicycle, vehicleCar)
- [x] 1.2 `RoutingProfile` with ParametrizeForFoot/Bicycle/Car already exists in `routing/RoutingProfile.h`
- [x] 1.3 Add speed map selection helper in JNI layer for vehicle-specific routing profiles (3 SP)

## 2. JNI Bridge — Profile marshaling

*Spec: routing-profile, route-calculation | Design: Data Flow*

- [x] 2.1 Add `Vehicle` Java enum to `libosmscout-client-java` (1 SP)
- [x] 2.2 Add `RoutingProfile` Java class with constructors and fields (1 SP)
- [x] 2.3 Add `calculateRouteAsync` overload accepting `RoutingProfile` in `OSMScoutClient.java` (1 SP)
- [x] 2.4 Implement JNI marshaling of `RoutingProfile` → C++ `osmscout::RoutingProfile` (3 SP)
- [x] 2.5 Implement vehicle-specific speed map selection in JNI routing callback (3 SP)
- [x] 2.6 Add `startNavigation` overload accepting `Vehicle` in `OSMScoutClient.java` (1 SP)
- [x] 2.7 Implement JNI `startNavigation` vehicle parameter → `JavaNavigationController` (2 SP)
- [x] 2.8 Ensure backward compatibility: existing no-profile overloads default to `Vehicle.CAR` (1 SP)

## 3. JavaScout UI — Vehicle selector

*Spec: route-input | Design: Data Flow*

- [x] 3.1 Add vehicle selector widget (segmented buttons or dropdown) to `RoutePanel` (3 SP)
- [x] 3.2 Add "Avoid tolls" and "Avoid ferries" checkboxes to `RoutePanel` (2 SP)
- [x] 3.3 Wire profile state through `RoutePanel` — store active `RoutingProfile` (1 SP)
- [x] 3.4 Pass `RoutingProfile` to `calculateRouteAsync` when profile is non-default (2 SP)
- [x] 3.5 Pass `Vehicle` to `startNavigation` when starting navigation session (1 SP)
- [x] 3.6 Show vehicle type in navigation status label (1 SP)
- [x] 3.7 Adapt next-turn overlay icon to vehicle type (bicycle/walking icon) (2 SP)

## 4. Testing

*Spec: route-calculation, routing-profile*

- [x] 4.1 Add bicycle route calculation test to `OSMScoutClientNavigationLiveTest` (3 SP)
- [x] 4.2 Add pedestrian route calculation test to `OSMScoutClientNavigationLiveTest` (3 SP)
- [x] 4.3 Add bicycle navigation session test (2 SP)
- [x] 4.4 Verify backward compatibility — existing car-only tests pass unchanged (1 SP)
