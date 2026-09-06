# Design: landuse-types-and-styles

## Context

See proposal.md - Why. `stylesheets/map.ost` defines import-time types; `stylesheets/include/landuse.oss` defines rendering. Pattern fills are PNG tiles loaded from icon directories at render time (`FillStyle::pattern` → `<pattern-path>/<pattern-name>.png`), replacing the solid fill at `patternMinMag` and above. `SymbolsAll` (Demos) renders stylesheet symbols standalone for visual scanning.

## Goals / Non-Goals

**Goals**
- Cover all `landuse=*` values that are concrete, wiki-documented, used >= 150 times, and not discouraged.
- Render new types with fills, labels, and pattern fills where a tile exists.
- Make pattern tiles visually scannable via `SymbolsAll`.

**Non-Goals**
- Not adding discouraged/deprecated/proposal values (see spec: Discouraged landuse values are not defined).
- Not changing the pattern-fill rendering mechanism in the backends (Skia/Cairo already support it).
- Not creating pattern tiles for types without an obvious universal symbol (e.g. no tile for `landuse_highway`-style abstract uses beyond what was authored).

## Decisions

### D1: Type selection criteria (usage >= 150, wiki-documented, not discouraged)
Follow the user's explicit criteria. Values like `school` (deprecated → `education`), `churchyard` (deprecated → `religious`), `harbour`/`depot`/`port` (discouraged), `traffic_island` (abandoned proposal), `civic_admin`/`shrubs`/`wasteland`/`plot`/`pasture` (proposals), `governmental`/`civic` (abandoned proposal), `paddy` (prefer `farmland=paddy`), `reservoir_watershed` (controversial), `yes` (pointless) are excluded.
- Alternative considered: adding everything with usage >= 150. Rejected — would import discouraged tags as first-class types.

### D2: Element types from the OSM wiki per-tag pages
Each tag's wiki page was visited; `NODE AREA` vs `AREA` in `map.ost` matches the wiki element table (e.g. `landuse=education` is node+area, `landuse=flowerbed` is area only).

### D3: Pattern tiles are opaque 14x14 PNGs, background = area fill color
Matches the existing convention (`cemetery.png`, `forest.png`, `scrub.png` are opaque tiles with the icon on a colored background). The tile background approximates the area fill color so the pattern reads as "area color + icon texture". Icons are bold geometric shapes that survive the 64px → 14px downscale (verified: 12–36% pixel coverage per tile).
- Alternative considered: transparent-background tiles. Rejected — the pattern replaces the fill entirely in the renderers, so a transparent tile would show the land layer beneath instead of the area color.

### D4: Pattern tile naming follows the symbol convention `<type>_<name>`
All symbols are named `<type>_<name>` (e.g. `leisure_slipway`, `historic_cemetery`). Pattern tiles now follow the same rule: `landuse_apiary.png`, `leisure_garden.png`, `natural_scrub.png`. This required renaming pre-existing tiles: `gardenpng` (typo, no extension) → `garden.png` → `leisure_garden.png`; `forest.png` → `landuse_forest.png`; `cemetery.png` → `landuse_cemetery.png`; `scrub.png` → `natural_scrub.png`. `garden.png` was kept as `leisure_garden.png` because `leisure_garden` (leisure.oss) also uses it; `scrub.png` became `natural_scrub.png` because only `natural_scrub` uses it.

### D5: `religious` tile uses a universal building-with-spire symbol
A cross is not a universal religious symbol. The tile depicts a building with a spire (place of worship), matching the user's review feedback.

### D6: SymbolsAll renders patterns via Cairo with nearest-neighbor scaling
Patterns are raster tiles, so only the Cairo backend renders them (SVG output is not meaningful for a PNG tile). Nearest-neighbor preserves the tile's actual pixels for faithful scanning. New `StyleConfig::GetPatternNames()` mirrors `GetSymbolNames()`, scanning `areaFillStyleSelectors` for non-empty `GetPatternName()`.
- Alternative considered: copying tiles unscaled. Rejected — 14x14 tiles are too small to scan; scaling to the 256px canvas matches the symbol output.

### D7: Pattern path resolution defaults to the standard icon directory
`--pattern-path` is repeatable; the default is `<stylesheet-dir>/../libosmscout/data/icons/14x14/standard`, which resolves correctly both from the build directory (`../stylesheets/standard.oss`) and from the repo root (`stylesheets/standard.oss`). If patterns exist but no path resolves, a warning is printed and the tool continues (exit 0).

## Risks / Trade-offs

- [Pattern tiles are hand-authored geometric shapes] → They are simple and verified via pixel analysis; they can be replaced with Inkscape-authored SVGs later without spec changes.
- [Renaming pre-existing tiles (`forest`, `cemetery`, `scrub`, `garden`) could break other stylesheets] → All pattern references were updated (`landuse.oss`, `cycle.oss`, `leisure.oss`, `natural.oss`); `CheckStyleSheet` tests cover every stylesheet.
- [Pattern replaces solid fill entirely at `patternMinMag`+] → Below the threshold the solid color shows; the tile background approximates the area color so the transition is subtle.
- [`GetPatternNames()` is a new public API] → Additive only; no behavioral change to existing rendering.

## Migration Plan

No runtime migration. Stylesheet + icon assets ship together; a database imported with the new types requires the matching `map.ost` at import time. Rollback: revert the stylesheet/icon changes; old databases remain readable.

## Open Questions

None.
