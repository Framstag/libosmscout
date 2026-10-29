# Tasks — client-java-avoid-tolls

## 1. The signal in the type configuration (spec: Toll ways are recorded in the routing index)

- [x] 1.1 Add a toll way feature beside `GradeFeature`: it reads the `toll` tag, treats `yes`/`true`/`1` as
  charging and everything else as not, and registers its tag name in `Initialize`.
- [x] 1.2 Register the feature so its name resolves when a type file declares it — **not** by adding it to
  every path-like type in code: that broke databases imported before the change (`Coordinate is not
  normalised` while reading `ways.dat`), because a feature the reader adds and the database never wrote
  shifts every way record.
- [x] 1.3 Give `ObjectVariantData` the masking accessors (`GetGrade()` returning the low bits, `IsToll()`
  returning the high bit) and keep `Read`/`Write` byte-identical.

## 2. The importer and the type files (spec: Toll ways are recorded in the routing index)

- [x] 2.1 Declare `FEATURE Toll` in `stylesheets/map.ost` and add `Toll` to the feature list of every
  car-routable way type in `stylesheets/map.ost` and `stylesheets/motorways.ost` (24 types).
- [x] 2.2 In `GenRouteDat`, read the toll feature from the way whose variant is being registered and record
  the bit beside the grade.
- [x] 2.3 Keep `operator==`/`operator<` of `ObjectVariantData` distinguishing a toll variant from a
  non-toll one, so the lookup table does not merge two differently priced roads — measured: before the
  detour way got its own type, both ways collapsed into one variant.

## 3. The profile (spec: avoidTolls prices a toll way higher)

- [x] 3.1 Mask the grade at every reader that indexes the speed table: `GetCosts`'s speed lookup and
  `CanUse(const RouteNode&, …)` (`GetTime2` reads the grade from a feature value, and `CanUse(const Area&)`
  likewise, so neither carries the bit).
- [x] 3.2 Add the `avoidTolls` option to the profile and apply the toll factor where the cost of an object
  variant is computed — not where a type's base speed is set, because the toll is per way.
- [x] 3.3 Fix the factor at 3.0 with the probe: the toll way is detoured when a free way of the same class
  exists, and it stays routable when it does not.

## 4. The bridge (spec: avoidTolls prices a toll way higher)

- [x] 4.1 Pass `avoidTolls` into the profile that `CreateRoutingProfile` builds and replace the log-only
  branch with a log line that names the factor.
- [x] 4.2 Confirm the flag reaches the profile through `calculateRouteAsync` on a database that records
  tolls.

## 5. Evidence owed before archive

- [x] 5.1 `evidence/TollProbe.cc` on `evidence/toll-test-map.osm`, imported with the changed importer:
  two object variants, one carrying the toll bit, grades still reading 1 in the low bits
  (`evidence/toll-probe-<date>.log`). Without the flag A to B is 3 584.7 m / 2 points; with it 7 575.1 m /
  3 points; 36 grid pairs, none lost.
- [x] 5.2 A database imported with the unmodified importer (the Bremen extract) loads, routes and shows no
  toll variants: 179 variants, 0 with a toll, 30 routes, none changed, none lost
  (`evidence/toll-probe-old-database-<date>.log`).
- [x] 5.3 Through the bridge (`evidence/AvoidTollsBridgeProbe.java`): 2 points without the flag, 3 with it,
  and the log line `avoiding toll roads (cost x3.0 …)`
  (`evidence/avoid-tolls-bridge-probe-<date>.log`).
- [ ] 5.4 Re-run the `client-java-avoid-unpaved` probe on a database imported with this change and confirm
  the grade ordering still holds. The re-imported test map shows grades intact in the census; a full
  re-import of a real extract is owed once this change and its predecessor are both on the branch.

## 6. Owed to the map provider

- [ ] 6.1 The app's maps are prebuilt downloads; the flag stays inert until the provider imports with a
  libosmscout that carries this change and a type file that declares `Toll`. This is a coordination item,
  not a code item, and it is why the issue splits.
