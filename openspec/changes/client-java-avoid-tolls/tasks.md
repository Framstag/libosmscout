# Tasks — client-java-avoid-tolls

## 1. The signal in the type configuration (spec: Toll ways are recorded in the routing index)

- [x] 1.1 Add a toll way feature beside `GradeFeature`: it reads the `toll` tag, treats `yes`/`true`/`1` as
  charging and everything else as not, and registers its tag name in `Initialize`.
- [x] 1.2 Provide the feature so its name resolves when a type file declares it — **not** by adding it to
  every path-like type in code: a feature injected in code for a database that never wrote it shifts every
  way record (measured: `Coordinate is not normalised` while reading `ways.dat`).
- [x] 1.3 Give `ObjectVariantData` the masking accessors (`GetGrade()` returning the low bits, `IsToll()`
  returning the high bit) and keep `Read`/`Write` byte-identical.

## 2. The importer and the type files (spec: Toll ways are recorded in the routing index)

- [x] 2.1 In `GenRouteDat`, read the toll feature from the way whose variant is being registered and record
  the bit beside the grade.
- [x] 2.2 Guard the reader: build the `TollFeatureValueReader` only when the type config declares `Toll`
  (`FeatureValueReader` asserts otherwise), and return `0` from `TollBit()` while the reader is null.
- [x] 2.3 Keep `operator==`/`operator<` of `ObjectVariantData` distinguishing a toll variant from a
  non-toll one, so the lookup table does not merge two differently priced roads.

## 3. The profile (spec: avoidTolls prices a toll way higher)

- [x] 3.1 Mask the grade at every reader that indexes the speed table: `GetCosts`'s speed lookup and
  `CanUse(const RouteNode&, …)`.
- [x] 3.2 Add the `avoidTolls` option to the profile and apply the toll factor where the cost of an object
  variant is computed — not where a type's base speed is set, because the toll is per way.
- [x] 3.3 Fix the factor at 3.0: the toll way is detoured when a free way exists, and it stays routable when
  it does not.

## 4. The bridge (spec: avoidTolls prices a toll way higher)

- [x] 4.1 Pass `avoidTolls` into the profile that `CreateRoutingProfile` builds and replace the log-only
  branch with a log line that names the factor.

## 5. The not-enabled state (guidelines/FileFormatVersion.md)

- [x] 5.1 Leave the `TypeConfig::Setup()` registration commented out, with the reason beside it.
- [x] 5.2 Leave the `FEATURE Toll` block in `stylesheets/map.ost` commented out and list `Toll` on no type;
  add the explanatory comment block to `stylesheets/motorways.ost`.
- [x] 5.3 Add the "A feature that exists but is not enabled" section to `guidelines/FileFormatVersion.md`.
- [x] 5.4 Bump no version: `FILE_FORMAT_VERSION` is unchanged.

## 6. Build and tests

- [x] 6.1 Build the touched libraries (`OSMScout`, `OSMScoutImport`, `osmscout_client_java`) one target at a
  time; no compile error or new warning.
- [x] 6.2 Run the existing routing-related tests (`RoutingProfileSpeedTest`, `MultiDBRoutingTest`,
  `RoutePostprocessorTest`, `NavigationAgentTest`) after rebuilding them; all pass, none weakened.
- [x] 6.3 `bash scripts/check-jni-signatures.sh`: this change adds no JNI native method.

## 7. Not reproducible in this checkout

- [ ] 7.1 The enabled-state measurements (a toll variant in the index, a detour chosen, a database that
  records a toll) cannot be reproduced here: `maps/` holds v26/v27 data with no routable database and no
  type file declares `Toll`. Reproducing them means enabling the feature, bumping `FILE_FORMAT_VERSION` and
  re-importing — the coordinated release this change deliberately does not make.
- [ ] 7.2 The user-visible effect waits on the map provider importing with a libosmscout that enables the
  feature and publishing the new version slot. This is a coordination item, not a code item.
