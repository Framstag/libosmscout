# Tasks — client-java-database-not-loaded-kind

## 1. The kind

- [x] 1.1 Add `kRouteFailureDatabaseNotLoaded=7`, `RouteFailureCause::DatabaseNotLoaded` and its
      `RouteFailureKind` branch to `libosmscout-client-java/src/route_failure_kind.h`, beside the existing
      kinds, with the reason the two empty-list cases are told apart.
      *(spec: `route-calculation` — RouteCallback Java interface, scenario "A calculation that outruns the
      database open reports that")*
- [x] 1.2 Report it at both `databases.empty()` branches of the route calculation in
      `libosmscout-client-java/src/OSMScoutClient.cpp`, leaving `kRouteFailureRoutingDataUnavailable` to the
      branches where the routing service cannot be opened.
      *(spec: `route-calculation` — RouteCallback Java interface, scenario "A calculation that outruns the
      database open reports that")*

## 2. The Java mirror

- [x] 2.1 Declare `KIND_DATABASE_NOT_LOADED = 7` in
      `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteCallback.java`, so the number that
      crosses the boundary is the same on both sides.
      *(spec: `route-calculation` — RouteCallback Java interface)*

## 3. The module's suite

- [x] 3.1 Extend `Tests/src/RouteFailureKindTest.cpp`: the `std::array` grows to eight causes with the new row,
      and the mirrored-number case pins `kRouteFailureDatabaseNotLoaded==7` and
      `RouteFailureKind(DatabaseNotLoaded)==7`.
      *(spec: `route-calculation` — RouteCallback Java interface)*
- [x] 3.2 Compile and run the case: the header is dependency-free, so the translation unit is compiled with the
      system compiler and its `static_assert`-free mapping is exercised by the same file the suite builds.
      Verify the record in `verification.md`.
