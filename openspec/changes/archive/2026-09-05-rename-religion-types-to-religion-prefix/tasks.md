## 1. Rename type definitions (spec: religion-type-definitions)

- [x] 1.1 Rename the 29 worship node types in `stylesheets/map.ost` from `<religion>_worship` to `religion_<value>` (e.g. `christian_worship` -> `religion_christian`, `buddhist_worship` -> `religion_buddhist`, `yazidi_worship` -> `religion_yazidi`) and verify each still matches the same `amenity=place_of_worship` + `religion=<value>` condition
- [x] 1.2 Rename the 8 building types in `stylesheets/map.ost` to the `religion_` prefix (`christian_church_building` -> `religion_christian_church_building`, `christian_cathedral_building` -> `religion_christian_cathedral_building`, `christian_chapel_building` -> `religion_christian_chapel_building`, `jewish_synagogue_building` -> `religion_jewish_synagogue_building`, `muslim_mosque_building` -> `religion_muslim_mosque_building`, `temple_building` -> `religion_temple_building`, `shrine_building` -> `religion_shrine_building`, `worship_building` -> `religion_building`) and verify each still matches the same condition
- [x] 1.3 Verify no old type names remain in `stylesheets/map.ost` — grep for `christian_worship`, `jewish_worship`, `muslim_worship`, `buddhist_worship`, `hindu_worship`, `shinto_worship`, `taoist_worship`, `sikh_worship`, `jain_worship`, `pagan_worship`, `zoroastrian_worship`, `chinese_folk_worship`, `multifaith_worship`, `bahai_worship`, `confucian_worship`, `vietnamese_folk_worship`, `ancestor_worship`, `animist_worship`, `antoinist_worship`, `benzhu_worship`, `caodaism_worship`, `shamanic_worship`, `scientologist_worship`, `self_realization_fellowship_worship`, `spiritualist_worship`, `tenrikyo_worship`, `unitarian_universalist_worship`, `voodoo_worship`, `yazidi_worship`, `temple_building`, `shrine_building`, `worship_building`, `christian_church_building`, `christian_cathedral_building`, `christian_chapel_building`, `jewish_synagogue_building`, `muslim_mosque_building` returns nothing

## 2. Update styles (spec: religion-type-definitions)

- [x] 2.1 Update the three building `[TYPE ...]` rule lists in `stylesheets/include/religious.oss` (lines ~142-168) to the renamed building types and verify the lists reference only `religion_*` names
- [x] 2.2 Update the 29 worship `[TYPE ...]` rules in `stylesheets/include/religious.oss` (lines ~189-247) to the renamed worship types and verify each rule still references its original symbol (`religion_christian_church_cross`, `religion_buddhist_dharma_wheel`, etc.)
- [x] 2.3 Verify no old type names remain in `stylesheets/include/religious.oss` — grep for the 37 old names returns nothing

## 3. Verification

- [x] 3.1 Run `cd build && ctest -R CheckStyleSheet --output-on-failure` and verify all stylesheet tests pass (validates `map.ost` against every `.oss` with `--warning-as-error`)
- [x] 3.2 Run `cd build && ctest -R StyleConfigSymbolsTest --output-on-failure` and verify the symbol test passes
- [x] 3.3 Run `cd build && cmake --build .` and verify the build compiles without errors
- [x] 3.4 Verify this change touched only `stylesheets/map.ost` and `stylesheets/include/religious.oss` (git diff on the other 8 modified stylesheet files contains zero religion tokens; the tree already had uncommitted work from other archived changes before this change)
