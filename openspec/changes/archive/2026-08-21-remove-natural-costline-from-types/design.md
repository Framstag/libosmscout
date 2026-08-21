# Design: Remove natural_coastline from natural type definitions

## Context

`natural_coastline` was added to `stylesheets/map.ost` by the `additional-natural-types` change. Coastline ways, however, are consumed by a dedicated import pipeline (`GenWaterIndex` in `libosmscout-import`) that builds the water/land tile index and coastline baseline. Defining the tag as a regular import-time type both duplicates that handling and creates a misleading `TypeConfig` entry. Motivation: see proposal.md — Why (PR #1769, karry comment).

## Goals / Non-Goals

**Goals:**
- Remove `TYPE natural_coastline` from `stylesheets/map.ost` so `natural=coastline` ways are not imported as a regular way type.
- Preserve knowledge: leave a comment at the removal site explaining the separate coastline handling.
- Drop the dead rendering rule referencing the removed type.
- Keep the `natural-type-definitions` spec in sync (delta in progress).

**Non-Goals:**
- No change to coastline import pipeline, `GenWaterIndex`, or baseline handling.
- No change to `basemap.ost` osmium filter comment (`wr/natural=coastline` there feeds the basemap water index — correct as is).
- No rework of other natural types.

## Decisions

1. **Comment placement: at removal site in `map.ost`.** Inline `//` comment replacing the removed block, referencing the separate water/land index + baseline handling. Rationale: future editors reading the type list see immediately why the tag is absent. Alternative considered: note in `natural.oss` only — rejected, file documents rendering not import types; map.ost is where someone would look to add the type.

2. **Remove the `natural_coastline` rule in `include/natural.oss`.** A rendering rule referencing a non-existent type is dead config. Rationale: stylesheets warn/behave unpredictably on unknown `[TYPE]` selectors; removing keeps natural.oss clean. Alternative: leave the rule dormant — rejected, dead config invites confusion and the spec scenario "New natural way types are rendered" lists coastline.

3. **Spec delta via MODIFIED requirements** for `natural-type-definitions` (water types table + rendering way-types scenario), plus explicit negative scenario "No coastline type in type config" so the removal is testable. Alternative: no spec change — rejected, spec would claim a type that no longer exists.

## Risks / Trade-offs

- [Style/type mismatch: `.oss` rules in other stylesheets (cycle.oss, etc.) may reference `natural_coastline` in the future] → grep before merge; current tree has only `natural.oss:172`, handled by Decision 2.
- [Databases imported with old stylesheets still contain `natural_coastline` objects] → backwards compatible: old DBs render whatever their types allow; new imports simply won't produce the type. No migration needed.
- [PR context says "should not be handled at this point" — future re-add possible] → comment states reason; re-adding is trivial if coastline handling changes.
