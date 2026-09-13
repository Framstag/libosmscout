## 1. Type definitions in map.ost

- [x] 1.1 Add NODE types `power_catenary_mast`, `power_portal`, `power_transformer`, `power_switch`, `power_terminal`, `power_insulator`, `power_connection`, `power_catenary_portal` to the power section of `stylesheets/map.ost` (spec: power-type-definitions — Object types) and verify each matches `"power"=="<value>"` with no name collisions (`grep -n "power_" stylesheets/map.ost`)
- [x] 1.2 Add WAY types `power_cable`, `power_circuit` to `stylesheets/map.ost` and verify they are declared WAY-only
- [x] 1.3 Add NODE AREA types `power_plant`, `power_heliostat`, `power_compensator`, `power_inverter`, `power_switchgear`, `power_converter` to `stylesheets/map.ost` and verify NODE AREA declaration
- [x] 1.4 Replace the `// TODO: power_plant` comment with the `power_plant` type definition (NODE AREA, `{Name, NameAlt}`, ADDRESS) and verify no TODO remains in the power section

## 2. Style definitions

- [x] 2.1 Add `NODE.ICON` styles for the new node types (`power_catenary_mast`, `power_portal`, `power_catenary_portal`, `power_terminal`, `power_connection`, `power_insulator`, `power_switch`, `power_compensator`, `power_transformer`, `power_inverter`, `power_switchgear`, `power_converter`, `power_heliostat`) in `stylesheets/include/man_made.oss` at `[MAG close-]`, reusing `SYMBOL power_tower`/`power_pole`, and verify the style file parses (spec: power-type-definitions — Style definitions)
- [x] 2.4 Add distinct `SYMBOL` definitions `power_catenary_mast`, `power_portal`, `power_transformer`, `power_switch`, `power_plant`, `power_heliostat` in `stylesheets/include/man_made.oss` and wire them as `NODE.ICON` for their types; verify via `SymbolsAll` that 8 power symbols render
- [x] 2.2 Add `WAY` styles for `power_cable` and `power_circuit` in `stylesheets/include/man_made.oss` cloned from `power_minor_line` and verify they render as grey lines
- [x] 2.3 Add AREA fill + label styles for `power_plant`, `power_heliostat`, `power_compensator`, `power_inverter`, `power_switchgear`, `power_converter` in `stylesheets/include/power.oss` at `@buildingMag-`/`@labelBuildingMag-`, cloned from `power_sub_station`/`power_generator`, and verify labels use `Name.name`

## 3. Verification

- [x] 3.1 Verify no existing power type/style was modified: `git diff stylesheets/map.ost stylesheets/include/man_made.oss stylesheets/include/power.oss` shows only additions
- [x] 3.2 Verify stylesheets load: run `Import` or a style-loading test (e.g. `Tests` style config test) against the standard style and confirm no parse errors
- [ ] 3.3 Render a test map containing the new power objects (catenary_mast, portal, transformer, cable, switch, plant, terminal, heliostat, insulator, circuit, connection, catenary_portal, compensator, inverter, switchgear, converter) and verify each is drawn
