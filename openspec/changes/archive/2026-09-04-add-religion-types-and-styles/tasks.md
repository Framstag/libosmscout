## 1. Type definitions

- [x] 1.1 Add NODE worship types for all wiki-documented religion values in `stylesheets/map.ost` (jewish, muslim, buddhist, hindu, shinto, taoist, sikh, jain, pagan, zoroastrian, chinese_folk, multifaith, bahai, confucian, vietnamese_folk, ancestor, animist, antoinist, benzhu, caodaism, shamanic, scientologist, self-realization_fellowship, spiritualist, tenrikyo, unitarian_universalist, voodoo, yazidi) and verify `StyleConfigSymbolsTest` loads map.ost
- [x] 1.2 Fix `muslin` → `muslim` typo in `muslim_mosque_building` and verify the type matches `religion=muslim`

## 2. Style definitions

- [x] 2.1 Add religion symbols (`religion_jewish_star_of_david`, `religion_muslim_crescent`, `religion_buddhist_dharma_wheel`, `religion_taoist_yin_yang`, `religion_shinto_torii`, `religion_pagan_pentagram`, `religion_place_of_worship`) in `stylesheets/include/religious.oss` and verify all render via `Demos/SymbolsAll`
- [x] 2.2 Rename existing `christian_church_cross` to `religion_christian_church_cross` and update all references
- [x] 2.3 Add NODE.TEXT + NODE.ICON styles for all worship types at very close zoom and verify `DumpOSS` loads standard.oss and cycle.oss without new warnings

## 3. Specs

- [x] 3.1 Create delta spec `specs/religion-type-definitions/spec.md` and verify `openspec validate --changes` passes
- [x] 3.2 Sync delta to main spec `openspec/specs/religion-type-definitions/spec.md` and verify `openspec validate --specs` passes
