## Why

Dark mode (the `daylight` stylesheet flag unset) is incomplete: many colors have no dark variant and stay bright (buildings `#d9d9d9`, shop `#d9bb98`, tourism `#c8c8db`, religious `#cccccc`, contours, grass, industrial), and generic buildings are hidden entirely instead of being dimmed. For night driving and navigation the map should be dim overall, show only features relevant for driving/navigation/orientation, and keep landmarks/POIs as icons without their area fills.

## What Changes

1. **Buildings dim + visible in dark**: `_building` and `_minorBuilding` flags stay `true` when `daylight` is unset (currently `false`), and `buildingColor`/`buildingBorderColor`/`buildingLabelColor` plus the minor building color get `IF daylight / ELSE darken(0.5)` branches in `standard.oss`, `cycle.oss`, `winter-sports.oss`.

2. **General color dimming**: add `IF daylight / ELSE darken(0.5)` branches to every color currently missing one: `grassColor`, `industrialColor`, contour colors, `postColor`, `cyclewayColor`, `shopColor`, `tourismColor`, `pitchColor`, `playgroundColor`, `religiousColor`, `religiousBuildingColor`, `officeColor`, `railwayColor`, `airwayColor`, `historicColor`, `militaryColor`, railway track colors, and any others found in the audit.

3. **Reduce number of colors**: consolidate colors that are near-duplicates of a base color (e.g. distinct light variants of the same hue) so dark mode has fewer distinct values to dim; derived colors (`darken(@base, x)`) stay derived.

4. **Detail reduction in dark**: new stylesheet flags `_shop`, `_tourism`, `_historic`, `_office`, `_landuse` default `true` in daylight and `false` in dark, gating the corresponding area-fill style blocks. General strategy: in dark mode keep NODE icons and labels for POIs and landmarks (monuments, viewpoints, fuel, parking, hospitals, traffic signals), hide special area fills of non-essential categories.

5. **Railway visible in dark**: `_railway` stays `true` when `daylight` is unset (currently `false`) so level crossings and rail context remain visible; railway track colors get dark variants.

## Capabilities

### New Capabilities
- `dark-mode-stylesheets`: Daylight-dependent color dimming and feature visibility in the `standard.oss`, `cycle.oss`, and `winter-sports.oss` stylesheets — dim colors when `daylight` is unset, show buildings/railway/POI icons dimmed, hide non-essential area fills.

### Modified Capabilities
<!-- None: no existing spec-level behavior changes. -->

## Impact

- `stylesheets/standard.oss` — FLAG section (`_building`, `_railway`, new `_shop`/`_tourism`/`_historic`/`_office`/`_landuse`), color daylight branches, color consolidation
- `stylesheets/cycle.oss`, `stylesheets/winter-sports.oss` — same FLAG/color changes
- `stylesheets/include/*.oss` — `IF _shop`/`_tourism`/`_historic`/`_office`/`_landuse` gating on area fills, daylight branches for colors in `shop.oss`, `tourism.oss`, `leisure.oss`, `religious.oss`, `office.oss`, `railway.oss`, `aerialway.oss`, `historic.oss`, `military.oss`, `natural.oss`, `landuse.oss`, `amenity.oss`, `roads.oss`
- No C++ code changes: the style system already evaluates `IF daylight` and flags at parse time
- No new dependencies
