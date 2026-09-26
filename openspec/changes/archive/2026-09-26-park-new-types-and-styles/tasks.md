# Tasks

## 1. Bring the branch up to date

- [x] 1.1 Switch to `further-types-styles-symbols-and-fixes`, fetch, merge `origin/master` and resolve the conflicts in `stylesheets/map.ost`, `stylesheets/motorways.ost` and the affected `stylesheets/include/*.oss` files; commit the merge. Verify: `git merge-base --is-ancestor origin/master HEAD` exits 0 and `git status` is clean. (Precondition for every scenario of `parked-type-definitions`, which has no requirement of its own.)
- [x] 1.2 Verify the merged tree before any parking: configure and build with cmake, then run the `StyleConfigSymbolsTest` and `RouteStyleColorsTest` cases, which load `stylesheets/map.ost`, `motorways.oss`, `standard.oss` and `winter-sports.oss`. Verify: the build succeeds and both cases pass, proving the merge resolved the stylesheets consistently before parking begins. (Requirement: Types that existed before the change stay fully active — baseline.)

## 2. Derive and record the parked set

- [x] 2.1 Derive the parked set from the merged tree — the type names the branch introduces, minus the nine renamed worship types that stay active — and write it sorted, one name per line, to `openspec/changes/park-new-types-and-styles/parked-types.txt`. Verify: the file has 854 lines, contains `highway_busway` and `landuse_apiary`, and does not contain `religion_christian` or `highway_motorway`. (Requirement: Unreleased definitions and their styles are retained in inactive form.)
- [x] 2.2 Record the parked set and its unpark condition in `TODO.md`, pointing at this change and at the `PARKED-NEW-TYPE` marker. Verify: the entry names the marker, the count 854 and the change directory. (Requirement: Parked state is locatable by one stable marker.)

## 3. Park the type definitions

- [x] 3.1 In `stylesheets/map.ost`, comment out every type definition of the 849 parked names defined there, bracketing each block with `PARKED-NEW-TYPE` markers. Verify: the number of markers equals 849, and no name listed in `parked-types.txt` still appears in an uncommented type definition line of that file. (Requirement: Unreleased definitions and their styles are retained in inactive form.)
- [x] 3.2 Do the same for the five parked definitions in `stylesheets/motorways.ost`: `highway_busway`, `highway_corridor`, `highway_escape`, `highway_proposed`, `highway_raceway`. Verify: all five are marked and no released definition carries a marker. (Requirement: Unreleased definitions and their styles are retained in inactive form.)
- [x] 3.3 Add a case to `Tests/src/TypeResolutionTest.cpp` that loads `stylesheets/map.ost` from the test data path, reads the `PARKED-NEW-TYPE`-marked names out of the source file, and asserts that none of them resolves while the representative released names `highway_motorway`, `amenity_restaurant`, `shop_supermarket`, `leisure_park`, `historic_castle` and the renamed `religion_christian` do. Verify: `ctest -R TypeResolutionTest` passes. (Requirements: Shipped type configuration contains only released types; Parked state is locatable by one stable marker.)

## 4. Park the style rules of parked types

- [x] 4.1 In the 17 affected `.oss` files, remove parked type names from active selectors: comment a selector line when it names only parked types, and when a selector list mixes parked and active types remove the parked names from the list and keep them as a commented line beneath it. Verify: a check over every shipped stylesheet finds no active type selector or group entry that is not a resolvable type and no active reference to a name in `parked-types.txt`. (Requirements: Shipped stylesheets load and resolve every type they reference; Unreleased definitions and their styles are retained in inactive form.)
- [x] 4.2 Park the 30 rules of `stylesheets/include/office.oss` like every other parked rule instead of switching the module off through `stylesheets/standard.oss`, so that each shipped file stays loadable on its own and no active rule names an unreleased type; state the reason in the commit message so the emptied module is not mistaken for a removal. Verify: `stylesheets/include/office.oss` keeps its colours and its rules are marked, the `MODULE "include/office"` line is untouched, and no `MODULE` line in any shipped stylesheet points at a stylesheet that references a parked type. (Requirement: Shipped stylesheets load and resolve every type they reference.)
- [x] 4.3 Verify the reporting case by loading a temporary stylesheet that references a parked type. Verify: the loader succeeds, reports `Unknown type '<name>'` naming that parked type and applies no rule. (Requirement: Shipped stylesheets load and resolve every type they reference — reporting scenario.)

## 5. Keep the active type set intact

- [x] 5.1 Check that every type which was active before the change still has at least one active style rule, and report any type that lost its styling through the selector-list surgery. Verify: the check reports no lost type; any hit is resolved before continuing. (Requirement: Types that existed before the change stay fully active.)
- [x] 5.2 Retarget the two new pattern enumeration cases in `Tests/src/StyleConfigSymbolsTest.cpp` from the parked `landuse_apiary` and `landuse_forestry` to the active `landuse_cemetery` and `leisure_garden`, keeping their two-entry assertions. Verify: `ctest -R StyleConfigSymbolsTest` passes. (Requirement: Symbol and pattern inspection covers the active styles only.)
- [x] 5.3 Verify that the renamed worship types are active under their new names only. Verify: `religion_christian`, `religion_temple_building` and `religion_building` resolve, while `christian_worship`, `temple_building` and `worship_building` do not. (Requirement: Types that existed before the change stay fully active — rename scenario.)

## 6. Integration verification

- [x] 6.1 Build the whole project with cmake and confirm it compiles without new warnings. Verify: the build succeeds and the warning output contains nothing beyond what the pre-parking build produced.
- [x] 6.2 Run the affected test cases `StyleConfigSymbolsTest`, `TypeResolutionTest`, `RouteStyleColorsTest` and `StyleLoadResilienceTest`. Verify: all of them pass.
- [x] 6.3 Load every shipped stylesheet — `standard.oss`, `cycle.oss`, `winter-sports.oss`, `public-transport.oss` and `motorways.oss` — through the `SymbolsAll` demo, falling back to the test suite's stylesheet loads if the demo invocation differs. Verify: each stylesheet loads, enumerates its symbols and patterns, and reports no warning naming a parked type; the warning names match the pre-change tree. (Requirement: Shipped stylesheets load and resolve every type they reference.)
- [x] 6.4 Run the full ctest suite. Verify: no regressions compared with the pre-parking run.
- [x] 6.5 Commit the parking work as two reviewable commits on top of the merge — one for the type and style parking, one for the test changes — with messages naming the `PARKED-NEW-TYPE` marker and stating that `include/office.oss` is emptied by parking its rules rather than by switching its module off. Verify: `git log --oneline` shows both commits and the working tree is clean.
