# Verification

Evidence for the tracked tasks of this change. Each task names the entry that closes it.

## Baseline (task 1.2)

Measured on the tree at the start of this change, `stylesheets/` as committed:

```
file                     active TYPE   PARKED-NEW-TYPE TYPE   DESC blocks   of which parked
map.ost                        640                849            11              0
motorways.ost                   23                  5             5              5
basemap.ost                     11                  2             0              0
contour_lines.ost                3                  0             0              0
                             ----               ----           ---             --
total                          677                856            16              5
```

`DESC` blocks describe 16 definitions: 6 features of the `FEATURES` block of `map.ost`, 5 types of
`map.ost` (`highway_track`, `highway_pedestrian`, `highway_path`, `highway_cycleway`,
`highway_footway`) and 5 types of `motorways.ost` (`highway_motorway`, `highway_motorway_trunk`,
`highway_trunk`, `highway_living_street`, `highway_service`). The 5 parked definitions of
`motorways.ost` carry commented `// DESC` blocks as well.

Types per top-level name of `map.ost`:

```
active   amenity 178 | natural 54 | shop 50 | sport 44 | tourism 37 | landuse 29 | place 21
         highway 21 | railway 18 | leisure 18 | barrier 17 | historic 16 | waterway 14
         route 14 | boundary 11 | routemaster 9 | religion 9 | military 7 | aeroway 7
         power 6 | building 4 | aerialway 3 | piste 3 | public_transport 3 | and 41 single
         names (bookkeeping types, the SRTM debug type, and single types such as `wood`,
         `speed_camera`)
parked   shop 328 | office 100 | historic 86 | man_made 73 | railway 31 | leisure 30
         religion 28 | piste 25 | waterway 22 | highway 20 | landuse 19 | barrier 19
         place 18 | power 16 | military 15 | aeroway 15 | aerialway 4
```

Names defined in more than one file (each occurrence needs its own `DESC`): `boundary_country`,
`boundary_state`, `boundary_county`, `boundary_administrative`(basemap only), `place_continent`,
`place_country`, `place_capitalcity`, `place_millioncity`, `place_ocean`, `place_sea` — defined in
both `map.ost` and `basemap.ost`.

The omission list `Tests/data/TypeDescriptionOmissions.txt` opens this change with 48 entries: the
41 `*_any` relation bookkeeping types, `public_transport_stop_area_group`, `address`, `border`,
`postal_code`, `organization`, the SRTM debug type `srtm_tile`, and `boundary_civil`. Every entry
names why no user-facing name exists for the type. The list is expected to grow while the
descriptions are written; it is the reviewed record of every deliberate omission, not a defect
list.

## Task log

| Task | Evidence |
|------|----------|
| 1.1 | `guidelines/TypeDescriptions.md` written; its rules 1–3 state the "Descriptions are worded for application users" requirement, rule 4 the "Each shipped language is worded on its own" requirement, rule 5 the "The shipped languages are a closed set" requirement, rule 6 the deliberate omission of an uncertain name. |
| 1.2 | `Tests/data/TypeDescriptionOmissions.txt` created with 48 entries and a reason per entry (verified: `grep -vc '^#\|^$'` = 48, column 2 present on every entry); the baseline counts above are the measurement of `stylesheets/` at the start of this change. |
| 2.1 | `Tests/src/TypeDescriptionCoverageTest.cpp` added and registered in `Tests/CMakeLists.txt` and `Tests/meson.build` (both set `TESTS_TOP_DIR`). It builds without warning and, run over the tree of this change, reports 1469 undescribed definitions with type, file and line (4407 failed assertions = 1469 x the three checks). Its second case, over the omission list, passes. It does not pass yet: 1470 definitions are still undescribed, which tasks 3.1 to 4.7 close. |
| 2.2 | The `DESC` block of `highway_motorway` in `stylesheets/motorways.ost` was removed and the test re-run: it reported `highway_motorway (motorways.ost:43) carries no DESC block`, `... carries no English description` and `... carries no German description`, and the failure count rose from 4407 to 4410. The block was restored, `git diff --stat stylesheets/` is empty and the count is 4407 again. |
| 3.1 | The `FEATURES` block of `stylesheets/map.ost` grew from 6 to 33 entries (+135 lines): the 6 that existed keep their descriptions, and 27 features a user encounters were added. `TypeDescriptionCoverageTest` reports no failure naming a feature (the failure total was unchanged at 4407, all of them types) and all 7 `CheckStyleSheet-*` tests pass, so every feature name of the block is known to the parser. Not in the block, because no user-facing text carries them: `Ele`, `Building`, `Color`, `Location`. |
| 3.2 | The 178 `amenity` types of `stylesheets/map.ost`: 164 carry a description now (+492 lines), and the 14 whose user-facing name is not established were added to the omission list with a reason (`amenity_college`, `amenity_college_building`, `amenity_prep_school`, `amenity_prep_school_building`, `amenity_mobile_money_agent`, `amenity_gambling`, `amenity_ranger_station`, `amenity_ranger_station_building`, `amenity_post_depot`, `amenity_post_depot_building`, `amenity_public_bath`, `amenity_public_bath_building`, `amenity_letter_box`, `amenity_lavoir`). `TypeDescriptionCoverageTest` reports no failure naming an `amenity` type, its omission-list case passes, the failure total fell from 4407 to 3873, and all 7 `CheckStyleSheet-*` tests pass. |
| 3.3 | The 50 `shop` and 2 `office` types of `stylesheets/map.ost`: 50 carry a description now (+150 lines), and `shop_coffee` and `shop_coffee_building` were added to the omission list with a reason. `TypeDescriptionCoverageTest` reports no failure for an active `shop` or `office` type — the 428 failures it still reports for these names are the parked definitions of group 4 — and its omission-list case passes. |
| 3.4 | The 54 `natural`, 29 `landuse` and 14 `waterway` types of `stylesheets/map.ost`: 93 `DESC` blocks were inserted for 92 table entries - `natural_bay` is defined twice, once actively in `map.ost` and once parked in `basemap.ost`, and both got the description - and `natural_fell`, `landuse_brownfield`, `landuse_greenfield` and `landuse_cemetery_sea` were added to the omission list with a reason. No active `natural`, `landuse` or `waterway` type is reported as undescribed any more, and `OSTAndOSSTest --warning-as-error stylesheets/basemap.ost stylesheets/basemap-render.oss` reports `OK` for both files, so the parked block of `basemap.ost` stayed well-formed. |
| 3.5 | The 37 `tourism`, 18 `leisure`, 44 `sport` and 16 `historic` types of `stylesheets/map.ost`: 112 carry a description now (+336 lines), and `leisure_common`, `sport_cricket_nets` and `sport_raquet` were added to the omission list with a reason. The set difference between the active names of these four keys and the names the coverage test still reports is empty, and all 7 `CheckStyleSheet-*` tests pass. |
| 3.6 | The 21 `highway`, 18 `railway`, 7 `aeroway`, 3 `aerialway` and 3 `piste` types of `stylesheets/map.ost`: 46 carry a description now (+138 lines); the 5 `highway` types that already had one keep it, and `railway_halt_any` stays in the omission list. The set difference between the active names of these keys and the names the coverage test reports is empty except for the names that `motorways.ost` defines (task 3.9), and all 7 `CheckStyleSheet-*` tests pass. |
| 3.7 | The 21 `place`, 11 `boundary`, 9 `religion`, 7 `military` and 6 `power` types: 52 table entries produced 62 `DESC` blocks (+186 lines) because 10 of these names are defined by `basemap.ost` as well as `map.ost`, and each definition needs its own block. The set difference between the active names of these keys and the names the coverage test reports is empty; `OSTAndOSSTest --warning-as-error stylesheets/basemap.ost stylesheets/basemap-render.oss` reports `OK` for both files, and all 7 `CheckStyleSheet-*` tests pass. |
| 3.8 | The 17 `barrier`, 3 `man_made`, 14 `route`, 9 `routemaster`, and the single `speed_camera`, `wood`, 4 `building` and 3 `public_transport` types: 44 table entries produced 45 `DESC` blocks (+135 lines) because `route_ferry` is defined twice by `map.ost`. `barrier_sally_port` and `barrier_kent_carriage_gate` were added to the omission list with a reason; the set difference between the active names of these keys and the names the coverage test reports is exactly those two. |
| 3.9 | The 23 `motorways.ost`, 11 `basemap.ost` and 3 `contour_lines.ost` types: 22 table entries produced 22 blocks (18 in `motorways.ost`, `basemap_boundary_country` in `basemap.ost`, 3 in `contour_lines.ost`); the other 10 `basemap.ost` names already got theirs in task 3.7, and the 5 descriptions `motorways.ost` carries are kept. With this, no active type of the shipped configuration is reported as undescribed: the set difference between every active name and the names the coverage test reports is empty, `TypeResolutionTest` passes and all 7 `CheckStyleSheet-*` tests pass. A defect of the description helper put the `DESC` block of the last definition of `motorways.ost` and of `contour_lines.ost` behind the closing `END`, which `TypeResolutionTest` then reported as `Symbol: EOF expected`; both blocks were moved before `END`, the helper now stops at a structural keyword, and no shipped `.ost` file carries a line after its `END`. |
| 4.1 | The 328 parked `shop` definitions of `stylesheets/map.ost`: 320 carry a commented `//   DESC` block inside their `PARKED-NEW-TYPE` block now (188 insertions in the first pass, 132 in the second), and `shop_tortilla`, `shop_tortilla_building`, `shop_variety_store`, `shop_variety_store_building`, `shop_chemist`, `shop_chemist_building`, `shop_energy` and `shop_energy_building` were added to the omission list with a reason. No parked `shop` definition is reported as undescribed, the strict `PARKED-NEW-TYPE` marker count of `map.ost` is unchanged at 849, `TypeResolutionTest` passes - so the marker contract, a marker followed by a commented type definition, still holds - and all 7 `CheckStyleSheet-*` tests pass. |
| 4.2 | The 100 parked `office` definitions of `stylesheets/map.ost`: 94 carry a commented `DESC` block now, and `office_guide`, `office_guide_building`, `office_quango`, `office_quango_building`, `office_religion` and `office_religion_building` were added to the omission list with a reason. The strict marker count stays 849. |
| 4.3 | The 86 parked `historic` and 73 parked `man_made` definitions: 143 blocks. 15 `historic` names and `man_made_bioswale` were added to the omission list with a reason each (`historic_bullaun_stone`, `historic_cattle_crush`, `historic_cattle_crush_building`, `historic_folly`, `historic_folly_building`, `historic_high_cross`, `historic_lavoir`, `historic_lavoir_building`, `historic_ogham_stone`, `historic_pound`, `historic_shieling`, `historic_shieling_building`, `historic_stecak`, `historic_tank`, `historic_tree_shrine`). The strict marker count stays 849. |
| 4.4 | The 31 parked `railway`, 30 `leisure`, 28 `religion` and 25 `piste` definitions: 93 blocks. 21 names were added to the omission list with a reason each - 2 `railway` (`defect_detector`, `owner_change`), `leisure_adult_gaming_centre`, 5 `piste` combinations, and 13 religions whose place of worship has no German name. The strict marker count stays 849. |
| 4.5 | The 22 parked `waterway`, 20 `highway`, 19 `landuse` and 19 `barrier` definitions: 74 of the 139 blocks of this pass. 6 names were added to the omission list with a reason each (`waterway_flowline`, `waterway_soakhole`, `highway_cyclist_waiting_aid`, `highway_trailhead`, `barrier_kissing_gate`, `barrier_hampshire_gate`). The strict marker count stays 849. |
| 4.6 | The 18 parked `place`, 16 `power`, 15 `military`, 15 `aeroway` and 4 `aerialway` definitions: 64 of the 139 blocks of this pass. 4 names were added to the omission list with a reason each (`place_plot`, `place_subcounty`, `aeroway_aircraft_crossing`, `aeroway_stopway`). The strict marker count stays 849. |
| 4.7 | The parked types of `motorways.ost` (5) and `basemap.ost` (2): `natural_mountain_range` received its block in this pass, `natural_bay` in task 3.4, and the 5 `motorways.ost` definitions keep the `//   DESC` blocks they already carried. With this the coverage test passes: `All tests passed (1 assertion in 2 test cases)` - no definition of the shipped configuration is left undescribed without a recorded reason. `TypeResolutionTest` passes and all 7 `CheckStyleSheet-*` tests pass. |
| 5.1 | `TypeInfo::GetLocalizedDescription(languageCode)` added to `libosmscout/include/osmscout/TypeConfig.h` and `libosmscout/src/osmscout/TypeConfig.cpp`: it returns the description of the requested language, the English description when that language is not carried, and an empty string when the type carries neither. `GetDescription()` keeps its exact behaviour. `Tests/src/TypeConfigDescriptionTest.cpp` covers the requested language, a requested language answered by English, a type without any description, an unknown type name and the enumeration of all descriptions; the test passes. |
| 5.2 | The same test file stores a configuration with and one without descriptions to a temporary directory and loads each into a fresh `TypeConfig`; the described one reports both languages after the load, the undescribed one reports no description for any type while loading without error. |
| 5.3 | The same test file registers a type the way an application registers an overlay of its own (`SetInternal().CanBeWay(true).CanBeArea(true).CanBeNode(true)` and `RegisterType`), and asserts that it carries no description for German and for English, and that a stored configuration does not carry it at all. |
| 6.1 | `OSMScoutClientBuilder.withDescriptionLanguage(code)` and `OSMScoutClient.setDescriptionLanguage(code)` added, and `objectTypeDescription` added to `PoiEntry`, `LocationEntry` and `ObjectDescription` (the new field gets a seventh constructor, the existing ones delegate with `null`). `libosmscout-client-java/src/OSMScoutClient.cpp` resolves the description through `ResolveTypeDescription`/`DescriptionLanguageOf` at every place a result is built: free-text hits, POI hits, structured location results and description results. `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientTypeDescriptionTest.java` covers the language on a running client, the non-null description field (and that a non-empty one never restates the type name), the English answer for a language the database does not carry, and a language change between searches: 4 cases run and pass against `TESTS/data/testregion`, the 5th (a database that carries descriptions) is skipped without `-Ddescription.test.db.dir`. The native library and the JAR build without a warning. Limitation: the committed test database carries no description for the types a search returns, so the Java side asserts the empty case and the fallback, while the described case is asserted in `TypeConfigDescriptionTest` (C++) and end to end in task 9.5. |
| 6.2 | The setting and the fields are documented in the JavaDoc of `OSMScoutClientBuilder.java`, `OSMScoutClient.java`, `PoiEntry.java`, `LocationEntry.java` and `ObjectDescription.java`, in a Common Patterns entry of `libosmscout-client-java/AGENTS.md` (the setting, the fallback order, "empty means show the technical type name", the additive fields) and in the `LocationEntry` row of its Key Data Types table. |
| 6.3 | Source-level check, no run: `libosmscout-binding/src/swig/osmscout/TypeConfig.i` wraps the header wholesale (`%include <osmscout/TypeConfig.h>` with `%shared_ptr(osmscout::TypeInfo)`), so the new `TypeInfo::GetLocalizedDescription(const std::string&)` is wrapped because it returns `std::string` (`std_string.i`), and both lookup forms are already reachable through `%include <std_unordered_map.i>` and `%template(StringStringUnorderedMap)` (`OSMScoutTypes.i:14`). No source change is needed. A build of the binding was not performed: `OSMSCOUT_BUILD_BINDING_JAVA=OFF` in this configuration and `swig` is not available in this environment. |
| 7.1 | `DescriptionLanguage` added to JavaScout: `forLocale` (German locale → `de`, everything else, including null, → `en`), `isSupported`, `other`, `label` and the display fallback `typeLabel`. `MainController` builds the client with `withDescriptionLanguage(DescriptionLanguage.forLocale(Locale.getDefault()))` and offers a "Type Descriptions" submenu (Deutsch / English) that calls `client.setDescriptionLanguage(...)` on the running client. `DescriptionLanguageTest` asserts the language for a German, an English and an unsupported locale, the supported set, the switch and the labels: 6 cases, all passing. |
| 7.2 | `PoiSearchOverlay.objectTypeLabel(item)` and `SearchOverlay.typeLine(item)` render the description of the object type and fall back to the technical name; the result list cells use them, and the `SearchResultCell` JavaDoc names the line accordingly. `PoiSearchOverlayTest` gained 4 cases and the new `SearchOverlayTest` 5 cases covering a described type, an empty description, a null description (a native library of an older vintage), a result without an object type and a result without a ref type. All pass. |
| 7.3 | `CandidatePickerOverlay.candidateRefLine` and `candidateName` name a candidate's object type by its description, falling back to the technical name; `CandidatePickerOverlayTest` (5 cases) covers a described candidate, an undescribed one, a null description, an unnamed candidate named by its type and a candidate that carries a name of its own. All pass. |
| 7.4 | `DescriptionOverlay` titles the dialog with `dialogTitle(description)`: the description of the described object's type when the database carries one, the technical type name otherwise, and the generic "Object Description" for an object whose type is unknown. `DescriptionOverlayTest` (4 cases) covers exactly those three outcomes. All pass. |
| 8.1 | `guidelines/TypeDescriptions.md` gained "Syntax of a description" (the `DESC` block of a type and of a feature, the parked form inside `PARKED-NEW-TYPE`) and "What a description does to the database", which states that a description is content and not layout, that `guidelines/FileFormatVersion.md` lists descriptions among the changes that keep `FILE_FORMAT_VERSION`, and that an existing database keeps opening but reports no description until it is regenerated. `Documentation/StyleSheetSyntax.txt` points at the guideline for the `.ost` files. The statement matches the format guideline; the end-to-end confirmation is task 9.5. |
| 8.2 | `AGENTS.md` names the description data where it names the stylesheets (`stylesheets/` row of the Repository Map and the `.ost` bullet of the Style Files section, both pointing at `guidelines/TypeDescriptions.md`), and `TODO.md` carries one new entry, `§104`, `category: java-bindings`, `class: bug`, `status: open`, for the Kotlin demo reader being out of sync with the type configuration format (version 19 against 27, features, groups and descriptions not kept, `SpecialType` read as a boolean). The `Clusters` block of `TODO.md` is generated by `cleanup-todo` and was not regenerated. |
| 9.1 | `cmake --build build` completes with 0 errors. Its 65 warnings name no file this change touches - they come from `libosmscout-client-qt/src/osmscoutclientqt/InputHandler.cpp` (Qt deprecation of `QTouchEvent::touchPoints()`), `MapWidget.cpp`, the vendored `libosmscout-map-skia/src/osmscoutmapskia/nanosvg.h` and obsolete tags of a generated Doxyfile. `OSMSCOUT_BUILD_CLIENT_JAVA=ON` in this configuration and `build/libosmscout-client-java/libosmscoutclientjava.jar` is produced. |
| 9.2 | `ctest -j 2 --output-on-failure`: the first run reported 159 of 160 passing with `OptimizeAreaWayIdsTest` as its only failure; that test passed on its own and a second full run passed 160 of 160. The failure is recorded as TODO §105. |
| 9.3 | `meson compile -C build-meson-check` (the configured Meson build directory of this checkout) completes with 0 errors, including `libosmscout-client-java/src/libosmscout_client_java.so`. |
| 9.4 | `mvn test -Dnative.lib.dir=<build>/libosmscout-client-java` in `JavaScout/` runs 267 tests with 0 failures and 0 errors; 23 are skipped (the ones whose database or display is not configured). `OSMScoutClientTypeDescriptionTest` runs one of its five cases without a database and four of five with `-Dpoi.test.db.dir=Tests/data/testregion`, so the new cases run rather than skip. `libosmscout-client-java` holds no Maven project (`mvn test` there fails with `MissingProjectException`), so it has no suite of its own; the Java tests of the bridge live in JavaScout. Recorded as TODO §106. |
| 9.5 | End to end through a database, with the new case in `Tests/src/TypeConfigDescriptionTest.cpp`: the shipped `stylesheets/map.ost` is read, the configuration is stored into the file a database carries and read back - `amenity_bank` keeps `bank` and `Bank`, the fallback answers `bank` for `fr`, the deliberate omission `amenity_gambling` reports no description, and the parked `shop_wool` is not part of a stored configuration at all. Additionally the committed database `Tests/data/testregion`, which the import tool generated, carries a description (`grep -c Wirtschaftsweg Tests/data/testregion/types.dat` = 1), so an import writes descriptions into `types.dat`. A fresh import was not run: the repository holds no raw OSM test data and downloading a region is beyond this change. |
| 9.6 | uncrustify: the two new test files are clean (they were formatted first, 34 and 16 lines of drift); `TypeConfig.h`, `TypeConfig.cpp` and `OSMScoutClient.cpp` fail the check before this change as well (the bridge file drifts by 8127 lines at HEAD and by 8180 with this change, and the remainder is declaration-comment alignment and continuation indentation inside blocks that already drift - TODO §88 covers the tree-wide drift). The additions were brought to what uncrustify asks wherever that does not depend on the drifted surroundings: the pointer star, one argument per line in the new calls, the comment alignment and the `jfieldID` padding. clang-tidy: no category in the new files that the existing tests do not carry (measured: 1 finding in `PatternLookupTest.cpp`, 18 in `StyleConfigSymbolsTest.cpp`, 67 in `DbJsonWriterTest.cpp`, 100 in `TypeResolutionTest.cpp`, against 7 and about 15 in the two new files); the categories the new files had introduced were fixed (swappable parameters, `starts_with`, string concatenation, the missing `<memory>` include). cppcheck is not installed here, so `scripts/cppcheck.sh` was not run. |
| 9.7 | This file: the task log above, the measured counts of the description work, the omission list with its reasons and the evidence for every task of this change. A second pass over the description data moved 35 parked blocks of `map.ost` above the section comment that introduces the following group of types, where they had been placed below it (a pure reordering: the two files hold the same non-blank lines, the same 1601 blank lines and the same 1387 `DESC` lines, and `diff <(grep -v '^$' map.ost | sort) <(grep -v '^$' /tmp/map-final.ost | sort)` is empty). The skill `.pi/skills/add-type-descriptions/` that this work produced places the blocks correctly; it was validated by reproducing the shipped files from the pre-change ones, which differ in nothing but the `FEATURES` block. The full suite passes after the reordering: `ctest -j 2` reports 160 of 160. |

### Result of the description work

```
description blocks inserted                                                    1410
  written as an active DESC line (released types and the new features)         620
  written as a commented // DESC line inside PARKED-NEW-TYPE (parked types)      790
blocks that already existed before this change                                   21
  map.ost: 6 features, 5 types | motorways.ost: 5 active + 5 parked types
described definitions                                                          1431
omitted definitions                                                             135
  from 134 entries of Tests/data/TypeDescriptionOmissions.txt; `address` is
  defined twice by map.ost, so its one entry covers two definitions
shipped definitions (TYPE + FEATURE) of the four .ost files                   1566
  33 features (6 pre-existing, 27 added) | 677 active types | 856 parked types
1431 + 135 = 1566
```

Measured with `git diff -U0 stylesheets/ | grep -c` on the `DESC`, `// DESC` and language lines, and
with the same extraction the coverage test uses for the definition count.

The descriptions were inserted with a transient helper that read a `<name><TAB><en><TAB><de>` table
and placed a `DESC` block at the end of the definition's body - as an active block, or as a commented
block inside the definition of a parked type. The helper and its tables are not part of the change
and were removed after the last group; the description data lives only in the `.ost` files.

`TypeDescriptionCoverageTest` reports `All tests passed (1 assertion in 2 test cases)`:
every definition of the shipped configuration is either described in both shipped languages or
listed in the omission list with a reason, and no entry of the omission list names a definition that
carries a description or does not exist.

### Gap at the start of the description work

```
definitions of the shipped type configuration (TYPE + FEATURE)      1539
  described in both shipped languages today                          21
  listed in Tests/data/TypeDescriptionOmissions.txt                  48
  undescribed                                                       1470
```

The 21 described definitions are the 6 features and 5 types of `map.ost` and the 10 type definitions of
`motorways.ost` (5 active, 5 parked). Two names are defined twice by the shipped configuration, so a
name can stand for more than one definition: `address` (twice in `map.ost`, both listed in the
omission list, so one entry covers two definitions) and `route_ferry` (twice in `map.ost`, both need
their own `DESC`).
