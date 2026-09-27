# Design: Dark Mode Stylesheets

## Context

See proposal.md - Why. The `daylight` stylesheet flag is evaluated at parse time by the OSS parser (`libosmscout-map/src/osmscoutmap/oss/Parser.cpp`): `IF daylight / ELSE` blocks select colors and flags, and `IF _<category>` blocks gate style rules. The app toggles the flag via `DBThread::ToggleDaylight()` which re-loads the stylesheet (`libosmscout-client/src/osmscoutclient/DBThread.cpp`). No C++ changes are needed — this is a stylesheet-only change.

Current state: `_building=false`, `_railway=false`, `_natural=false`, `_leisure=false` when `daylight` is unset; many colors lack `IF daylight` branches and stay bright.

## Goals / Non-Goals

**Goals**
- Dark mode renders dim colors (darken 0.5 from daylight values) for everything visible
- Buildings and railway render dimmed in dark mode (not hidden); minor buildings (garages) also render dimmed
- Non-essential area fills (shop, tourism, historic, office, landuse, leisure, natural) hidden in dark mode
- POI and landmark node icons/labels remain visible in dark mode
- Fewer distinct colors: consolidate near-duplicate literals into base + derivation

**Non-Goals**
- No C++/renderer changes (no `ColorPostprocessor` wiring, no new API)
- No changes to the `daylight` toggle mechanism itself
- No changes to daylight-mode rendering (must stay identical)
- No new icons or symbols

## Decisions

### D1: Per-category flags gate area fills; icons stay ungated

The general strategy "keep icons, hide area fills" is implemented by wrapping only AREA fill rules in `IF _<category>` blocks, leaving NODE icon/label rules outside. `_building` and `_shop` are orthogonal: `_building=true` in dark shows shop *buildings* (dimmed, orientation anchors) while `_shop=false` hides shop *area fills*. `_minorBuilding` stays `true` in dark so garages render dimmed like other buildings.

- **Alternative A (chosen)**: per-category flags (`_shop`, `_tourism`, `_historic`, `_office`, `_landuse`) added to the FLAG sections of `standard.oss`, `cycle.oss`, `winter-sports.oss`, gating area-fill blocks in the include files.
- **Alternative B**: single `_detail` flag gating all non-essential fills. Rejected: loses per-category control, and the existing `_building`/`_natural`/`_railway`/`_leisure` scheme already establishes per-category flags.
- **Alternative C**: hide via magnification (raise min mag for non-essential fills in dark). Rejected: magnification is zoom-based, not mode-based; would require duplicating every rule.

### D2: Per-color `IF daylight / ELSE darken(0.5)` branches

Each color missing a dark variant gets an explicit branch, following the existing pattern (e.g. `amenity.oss`).

- **Alternative A (chosen)**: explicit per-color branches in the stylesheets. Matches existing convention, per-color control, stylesheet-only.
- **Alternative B**: global `ColorPostprocessor` (a `Color (*)(const Color&)` hook already present in `StyleConfig::Load`) that darkens every color when `daylight` is unset. Rejected: requires C++ change in `DBThread::makeStyleConfig`, dims already-dark colors (black bridges, dark browns) with no per-color escape, and diverges from the established stylesheet pattern.
- **Alternative C**: `darken` applied at rule level (each `AREA { color: ... }` gets a dark variant). Rejected: duplicates rules massively; colors are the right abstraction level.

### D3: Color consolidation via base + derivation

Near-duplicate literals (same hue, different lightness) collapse to one base color with `darken`/`lighten` derivations, so dark mode dims one value instead of several.

- **Alternative A (chosen)**: audit each include file; where literals are within a small hue family, keep the canonical daylight literal and derive the rest (`darken(@base, x)`).
- **Alternative B**: leave literals, add branches to each. Rejected: multiplies the number of values to maintain and dim; contradicts the "reduce number of colors" goal.

### D4: Railway visible in dark

`_railway=true` when `daylight` is unset; railway track colors (`#b3b3b3`, `#939393`, `#777777` in `railway.oss`) get dark branches.

- **Alternative A (chosen)**: keep `_railway=true`, dim track colors. Level crossings and rail context remain visible for navigation.
- **Alternative B**: keep `_railway=false` (current). Rejected: hides rail context needed for level-crossing awareness at night.

## Risks / Trade-offs

- [Dimmed colors too dark to distinguish] → darken(0.5) matches existing convention; verify visually per category; adjust factor per color if needed.
- [Icons without fills look orphaned in dark] → icons are small and self-contained; verify landmark icons remain legible on dimmed background.
- [Color consolidation changes daylight appearance] → consolidation must preserve the daylight literal values exactly; only derivation structure changes.
- [Flag gating misses a rule] → audit each include file for AREA rules of gated categories; the `_building` gating already exists as a template.
- [Cycle/winter-sports stylesheets drift from standard] → apply the same FLAG/color changes to all three base stylesheets.

## Migration Plan

1. Add new flags to FLAG sections of `standard.oss`, `cycle.oss`, `winter-sports.oss`; flip `_building`/`_railway` to `true` in the dark branch.
2. Add `IF daylight / ELSE darken(0.5)` branches to all audited colors in base stylesheets and include files.
3. Wrap non-essential area-fill rules in `IF _<category>` blocks; leave icon/label rules ungated.
4. Consolidate near-duplicate color literals.
5. Verify: load each stylesheet with `daylight` true and false (e.g. via `StyleConfig` load in a test or the app's Ctrl+D toggle); confirm daylight rendering unchanged and dark rendering dim.

Rollback: revert stylesheet files; no code or data migration involved.

## Open Questions

- Exact darken factor per color family (0.5 default; some may need 0.3/0.7) — tunable during implementation without changing the approach.
- Which specific landmark types count as "landmarks" for the keep-icons rule (historic monuments, tourism viewpoints, natural peaks) — resolved during implementation by auditing NODE icon rules of the gated categories.
