## 1. Flag Scheme Changes

- [x] 1.1 In `stylesheets/standard.oss` FLAG section: set `_building`, `_minorBuilding`, and `_railway` to `true` in the `ELSE` (dark) branch; add new flags `_shop`, `_tourism`, `_historic`, `_office`, `_landuse` with `true` in the `IF daylight` branch and `false` in the `ELSE` branch. Verify: `OSTAndOSSTest stylesheets/map.ost stylesheets/standard.oss` reports OK (spec: Buildings render dimmed in dark mode; Railway renders dimmed; Non-essential area fills hidden).
- [x] 1.2 Apply the same FLAG changes to `stylesheets/cycle.oss` and `stylesheets/winter-sports.oss`. Verify: `OSTAndOSSTest` reports OK for both (spec: same as 1.1).

## 2. Color Dimming — Base Stylesheets

- [x] 2.1 Add `IF daylight / ELSE darken(0.5)` branches to `buildingColor`, `buildingBorderColor`, `buildingLabelColor` in `stylesheets/standard.oss`, `stylesheets/cycle.oss`, `stylesheets/winter-sports.oss`; add a dark branch to the minor building color (`#bcbcbc` in `stylesheets/include/buildings.oss`). Verify: `OSTAndOSSTest` reports OK; dark branch values are darkened (spec: Dark mode dims rendered colors; Buildings render dimmed).
- [x] 2.2 Add dark branches to remaining base colors in `stylesheets/standard.oss`: `grassColor`, `industrialColor`, `majorContourColor`, `mediumContourColor`, `minorContourColor`, `postColor`, `cyclewayColor`, `thinSecondaryColor`, `thinTertiaryColor`, `damColor`, `natureReserveColor`. Verify: `OSTAndOSSTest` reports OK (spec: Dark mode dims rendered colors).

## 3. Color Dimming — Include Files

- [x] 3.1 Add dark branches to colors in `stylesheets/include/shop.oss` (`shopColor`), `tourism.oss` (`tourismColor`), `leisure.oss` (`pitchColor`, `playgroundColor`), `religious.oss` (`religiousColor`, `religiousBuildingColor`), `office.oss` (`officeColor`), `railway.oss` (`railwayColor` + track colors `#b3b3b3`/`#939393`/`#777777`), `aerialway.oss` (`airwayColor`), `historic.oss` (`historicColor`), `military.oss` (`militaryColor`). Verify: `OSTAndOSSTest` reports OK for standard.oss and cycle.oss (spec: Dark mode dims rendered colors; Railway renders dimmed).
- [x] 3.2 Audit remaining include files (`amenity.oss`, `landuse.oss`, `natural.oss`, `roads.oss`, `aeroway.oss`, `man_made.oss`, `power.oss`, `sport.oss`, `waterway.oss`, `basemap.oss`, `place.oss`) for any color literal without a dark branch; add branches where the color is light enough to glow in dark. Verify: grep audit shows no light literals without dark branches (spec: Dark mode dims rendered colors).

## 4. Area Fill Gating

- [x] 4.1 Wrap shop area-fill rules in `stylesheets/include/shop.oss` in `IF _shop { ... }`, leaving NODE icon and label rules ungated. Verify: `OSTAndOSSTest` reports OK; `--analyze` shows shop node types still styled (spec: Shop area fills hidden; POI icons visible).
- [x] 4.2 Wrap tourism area-fill rules in `stylesheets/include/tourism.oss` in `IF _tourism { ... }`, leaving landmark NODE icons (viewpoints, attractions) ungated. Verify: `OSTAndOSSTest` reports OK (spec: Tourism area fills hidden; Landmark icons visible).
- [x] 4.3 Wrap historic area-fill rules in `stylesheets/include/historic.oss` in `IF _historic { ... }`, leaving monument/memorial NODE icons ungated. Verify: `OSTAndOSSTest` reports OK (spec: Historic area fills hidden; Landmark icons visible).
- [x] 4.4 Wrap office area-fill rules in `stylesheets/include/office.oss` in `IF _office { ... }`. Verify: `OSTAndOSSTest` reports OK (spec: Office area fills hidden).
- [x] 4.5 Wrap landuse area-fill rules in `stylesheets/include/landuse.oss` in `IF _landuse { ... }`. Verify: `OSTAndOSSTest` reports OK (spec: Landuse area fills hidden).
- [x] 4.6 Verify `_natural` and `_leisure` gating already hides natural/leisure fills in dark; confirm no natural/leisure NODE icons (peaks, viewpoints) are inside the gated blocks. Verify: grep audit of `natural.oss`/`leisure.oss` shows icon rules outside `IF _natural`/`IF _leisure` (spec: Natural/leisure area fills hidden; Landmark icons visible).

## 5. Color Consolidation

- [x] 5.1 Audit color literals per include file; consolidate near-duplicate literals (same hue family) into one base color with `darken`/`lighten` derivations, preserving exact daylight values. Verify: daylight rendering values unchanged (compare before/after color tables) (spec: Color consolidation preserves daylight appearance).
- [x] 5.2 Ensure consolidated derived colors compute from the dimmed base in dark mode. Verify: `OSTAndOSSTest` reports OK; spot-check derived values in dark branch (spec: Derived colors follow their base).

## 6. Verification

- [x] 6.1 Run `OSTAndOSSTest` against `map.ost` + all stylesheets (`standard.oss`, `cycle.oss`, `winter-sports.oss`, `railways.oss`, `motorways.oss`, `public-transport.oss`, `boundaries.oss`). Verify: all report OK (rule: stylesheets load without errors).
- [x] 6.2 Run `ctest -R CheckStyleSheet` in the build directory. Verify: 7/7 pass (rule: existing tests still pass).
- [x] 6.3 Load each stylesheet with `daylight` flag true and false (e.g. via a small test harness or the app's Ctrl+D toggle) and compare: daylight rendering identical to pre-change; dark rendering dim, buildings/railway visible, non-essential fills absent, POI/landmark icons present. Verify: visual comparison (spec: Daylight mode unchanged; all dark-mode requirements).
- [x] 6.4 Run full `ctest -j 2 --output-on-failure` in the build directory. Verify: no regressions (rule: existing tests still pass).
