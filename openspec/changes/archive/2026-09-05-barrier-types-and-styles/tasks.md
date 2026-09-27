# Tasks: barrier-types-and-styles

## 1. Type definitions in map.ost

- [x] 1.1 Add `barrier_kerb` (NODE WAY), `barrier_guard_rail` (WAY), `barrier_handrail` (WAY), `barrier_chain` (NODE WAY), `barrier_jersey_barrier` (NODE WAY), `barrier_log` (NODE WAY), `barrier_rope` (NODE WAY), `barrier_avalanche_protection` (NODE WAY AREA) TYPE definitions to the barrier section of `stylesheets/map.ost` (spec: Linear barrier types) and verify each type name is unique in the file
- [x] 1.2 Add `barrier_swing_gate`, `barrier_wicket_gate`, `barrier_kissing_gate`, `barrier_height_restrictor`, `barrier_turnstile`, `barrier_sliding_gate`, `barrier_hampshire_gate`, `barrier_border_control`, `barrier_planter`, `barrier_debris`, `barrier_full_height_turnstile` TYPE definitions (all NODE, marked IGNORE) to the barrier section of `stylesheets/map.ost` (spec: Node barrier types) and verify each type name is unique in the file
- [x] 1.3 Verify no TYPE definitions were added for discouraged/undocumented values (`barrier=yes`, `embankment`, `wire_fence`, `door`) by grepping `stylesheets/map.ost` for the corresponding type names (spec: Discouraged and undocumented barrier values)

## 2. Style definitions in man_made.oss

- [x] 2.1 Add wall-like WAY line rules for `barrier_kerb` and `barrier_jersey_barrier` in the `[MAG closer-]` block of `stylesheets/include/man_made.oss` (spec: Way rendering for new linear barrier types) and verify the rules reference `@wallColor`
- [x] 2.2 Add fence-like WAY line rules for `barrier_guard_rail`, `barrier_handrail`, `barrier_log`, `barrier_avalanche_protection` in the `[MAG veryClose-]` block of `stylesheets/include/man_made.oss` (spec: Way rendering for new linear barrier types) and verify the rules reference the fence color `#aaaaaa`
- [x] 2.3 Add dashed WAY line rules for `barrier_chain` and `barrier_rope` in the `[MAG veryClose-]` block of `stylesheets/include/man_made.oss` (spec: Way rendering for new linear barrier types) and verify the rules use `dash: 1,1`

## 3. Verification

- [x] 3.1 Verify the stylesheets parse by running the style validation on `map.ost` + `standard.oss` and confirming no type/style parse errors
- [x] 3.2 Verify the build compiles without errors (`cmake --build build` for the affected targets)
- [x] 3.3 Verify existing tests still pass (`cd build && ctest -j 2 --output-on-failure`)
- [x] 3.4 Re-import a test database and verify the new types exist in the `TypeConfig` (e.g. via `DumpData` or a type listing) and that ways render with the expected lines
