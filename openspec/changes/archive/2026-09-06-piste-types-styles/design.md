## Context

The libosmscout stylesheets consist of two file types: `.ost` type definitions (`map.ost`, single file, no includes) and `.oss` style modules (`stylesheets/include/*.oss`, included by `standard.oss` and friends). Piste types live in the "Winter sports" section of `map.ost` (lines ~767–781); piste styles live in `include/piste.oss`, which is included by `winter-sports.oss`. Rendering priority is controlled by `GROUP` statements in each `.oss` root file; only `winter-sports.oss` declares a piste GROUP. See proposal.md - Why for motivation.

## Goals / Non-Goals

**Goals:**
- Add 26 new piste type definitions to `map.ost` with object types matching the OSM wiki
- Add style definitions and symbols for the new types in `include/piste.oss`
- Extend the piste GROUP priority line in `winter-sports.oss`
- Keep all existing piste types/styles byte-identical

**Non-Goals:**
- No changes to `basemap.ost` (basemap import has no piste types today)
- No changes to C++ code, import pipeline, or rendering backends
- No new symbols for linear piste types (reuse the way-color approach of the existing downhill styles)
- No support for `piste:type=*` values below 150 taginfo uses that are not documented on the wiki, or marked discouraged (`yes`)

## Decisions

### D1: Add types inline in `map.ost` rather than splitting into a separate include file
`map.ost` has no `INCLUDE` mechanism — all type definitions are inline in one file. The winter sports section already holds 3 types; the new ones are appended after `piste_downhill_advanced`, keeping the section contiguous.
- **Alternative considered**: creating a `piste.ost` include — rejected because the OST format in this project does not support includes; all other feature groups (railway, natural, etc.) are inline in `map.ost`.

### D2: Difficulty-specific downhill types before the generic fallback
Type matching in `TypeConfig::GetWayAreaType` iterates types in declaration order and returns the first match. The difficulty-specific types (`piste_downhill_novice` … `piste_downhill_extreme`) are therefore declared before the generic `piste_downhill` fallback, so a downhill way with a known difficulty gets the difficulty-specific type and only untagged-difficulty ways fall through to the generic type.
- **Alternative considered**: no generic fallback — rejected: downhill ways without `piste:difficulty` (a common case) would remain untyped.

### D3: Order-variant semicolon combinations share one type
`piste:type=nordic;hike` and `piste:type=hike;nordic` are semantically identical; taginfo counts them separately (1102 and 235 uses). Each combination gets one type with an OR condition covering both orders, e.g. `piste_nordic_hike = WAY (("piste:type"=="nordic;hike") OR ("piste:type"=="hike;nordic"))`. This keeps the type count down while covering all >=150-use values.
- **Alternative considered**: one type per value order — rejected: doubles the number of near-identical combo types.

### D4: Colors follow OpenSnowMap, downhill difficulty colors follow the wiki
The wiki difficulty table maps novice→green, easy→blue, intermediate→red, advanced→black, expert→orange, freeride→yellow, extreme→near-black. OpenSnowMap's `downhill.mss`/`others.mss`/`nordic.mss` provide hex values for these and for the other piste types (nordic `#298DB2`, skitour `#C42C1C`, hike `#E88193`, sled `#BAFF3A`, sleigh `#8F54D8`, ice_skate `#6FC4D0`, snow_park `#0F6DD3`, playground `#A6E05B`, ski_jump `#655ABE`, fatbike `#7359E6`, connection `#373737`). The existing libosmscout downhill colors (`#5050ff`, `#ff5050`, `#606060`) are kept unchanged; new difficulty variants use the wiki/OpenSnowMap palette.
- **Alternative considered**: inventing a new palette — rejected: OpenSnowMap is the de-facto reference renderer for pistes and its colors are familiar to users.

### D5: Node types get simple geometric symbols
`snow_park`, `playground`, and `snowkite` are NODE AREA types. Following the waterway module pattern (simple geometric symbols in a module color), each gets a small `SYMBOL` definition and `NODE.ICON`/`AREA.ICON` rules at close zoom.
- **Alternative considered**: no icons, text only — rejected: the wiki provides pictograms for these features and the project convention is to render POI-like nodes with symbols.

### D6: GROUP priority update only in `winter-sports.oss`
Only `winter-sports.oss` declares a piste GROUP (verified by grep across `standard.oss`, `cycle.oss`, `public-transport.oss`). The GROUP line is extended with all new piste types.
- **Alternative considered**: adding piste GROUPs to the other `.oss` files — rejected: they do not reference piste types today; adding them would change rendering priority of existing pistes in those styles.

## Risks / Trade-offs

- [New types render at wrong priority in a style file that was missed] → Mitigation: grep confirmed only `winter-sports.oss` has a piste GROUP; `CheckStyleSheet-*` tests validate every `.oss` against `map.ost`.
- [Symbol shapes are approximate (simple geometric forms)] → Mitigation: acceptable for a data-driven stylesheet; symbols follow the existing module aesthetic and can be refined later without spec changes.
- [`snowshoe` has no wiki page (404) and `snowkite`/`snowmobile`/`halfpipe` have low usage] → Mitigation: `snowshoe` (428 uses) and `snowkite`/`snowmobile`/`halfpipe` are documented on the wiki Pistes page; object types taken from the wiki table; if the wiki later changes element documentation, the type line is a one-line change.
- [Semicolon combination types are fragile against new order variants] → Mitigation: only combinations with >=150 uses are added; new combinations can be added later as one-line type additions.

## Migration Plan

- No data migration: stylesheets are consumed at import/render time; existing databases re-imported with the new `map.ost` pick up the new types automatically.
- Rollback: revert the stylesheet files; no code or schema changes to undo.

## Open Questions

None.
