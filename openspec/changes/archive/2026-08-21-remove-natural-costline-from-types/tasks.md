# Tasks: Remove natural_coastline from natural type definitions

## 1. Stylesheet changes

- [x] 1.1 Remove `TYPE natural_coastline` block from `stylesheets/map.ost` and replace with a `//` comment stating that coastline ways are handled separately by the water/land index and baseline pipeline (spec: Water natural types — "No coastline type in type config"). Verify: `grep -n "natural_coastline" stylesheets/map.ost` returns only the comment, and the commented block contains no active type definition.
- [x] 1.2 Remove the `[TYPE natural_coastline]` rendering rule from `stylesheets/include/natural.oss` (spec: "New natural way types are rendered"). Verify: `grep -n "natural_coastline" stylesheets/` finds no rule outside comments.

## 2. Spec updates

- [x] 2.1 Apply the `natural-type-definitions` delta to `openspec/specs/natural-type-definitions/spec.md`: drop the coastline row from the Water natural types table and the "Coastline type exists in type config" scenario; drop `natural_coastline` from the "New natural way types are rendered" scenario list (spec: delta file). Verify: `grep -rn "natural_coastline" openspec/specs/natural-type-definitions/spec.md` returns nothing.

## 3. Verification

- [x] 3.1 Validate change artifacts: `openspec validate remove-natural-costline-from-types --type change` passes (spec: all). Verify: command exits 0.
- [x] 3.2 Confirm no remaining code references: `grep -rn "natural_coastline" libosmscout* Demos OSMScout2 Tests` empty (spec: all). Verify: no matches.
- [x] 3.3 Build stylesheet parser + test binary: `cmake --build build --target OSTAndOSSTest`. Verify: build completes without errors.
- [x] 3.4 Run stylesheet test suite: `cd build && ctest -R CheckStyleSheet --output-on-failure` (spec: all). Verify: all 7 CheckStyleSheet tests pass.
