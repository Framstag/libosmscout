# Tasks: add-military-types-and-styles

## 1. Research and type selection

- [x] 1.1 Fetch OSM wiki [Key:military](https://wiki.openstreetmap.org/wiki/Key:military) and the `Template:Map_Features:military` element table to enumerate documented values and their NODE/WAY/AREA element types. Verify: list of 14 documented values with element types recorded.
- [x] 1.2 Fetch taginfo API (`/api/4/key/values?key=military`) for usage fractions; identify values >= 0.10% not covered by the wiki template. Verify: threshold list matches spec tables in `specs/military-type-definitions/spec.md`.
- [x] 1.3 Fetch taginfo per-element stats (`/api/4/tag/stats`) for undocumented values to derive element types. Verify: element types in spec match taginfo node/way/relation dominance.
- [x] 1.4 Confirm discouraged values (`military=yes`, `military=abandoned`, deprecated `military=naval_base`) are excluded from new types. Verify: spec "Discouraged and undocumented military values" requirement.

## 2. Type definitions (spec: military-type-definitions)

- [x] 2.1 Add node-only types `military_cannon`, `military_checkpoint`, `military_embrasure` to `stylesheets/map.ost` Military section. Verify: `grep "TYPE military" stylesheets/map.ost` shows them; `OSTAndOSSTest` loads map.ost OK.
- [x] 2.2 Add node-area types `military_ammunition`, `military_base`, `military_nuclear_explosion_site`, `military_office`, `military_police`, `military_radar`; `military_office` in `GROUP office, routingPOI`. Verify: OSTAndOSSTest loads OK; `--analyze` shows Area+Node rows for each.
- [x] 2.3 Add way/way-area types `military_cordon` (WAY), `military_road` (WAY), `military_obstacle_course` (WAY AREA), `military_shelter` (WAY AREA), `military_trench` (NODE WAY). Verify: OSTAndOSSTest loads OK; `--analyze` shows Way rows.
- [x] 2.4 Add area-only type `military_training_area` (AREA). Verify: OSTAndOSSTest loads OK; `--analyze` shows Area row only.

## 3. Style definitions (spec: military-type-definitions)

- [x] 3.1 Add `militaryBorderColor` const and symbols `military_checkpoint`, `military_cannon`, `military_radar`, `military_embrasure`, `military_shelter`, `military_police`, `military_trench` to `stylesheets/include/military.oss`. Verify: no unknown-symbol warnings from OSTAndOSSTest.
- [x] 3.2 Extend city-zoom area fill and close-zoom label groups with `military_ammunition`, `military_base`, `military_nuclear_explosion_site`, `military_training_area`. Verify: `--analyze` shows Area rows 11-18 for these types.
- [x] 3.3 Add dashed line styles for `military_trench`, `military_cordon`, `military_road`, `military_shelter` at close zoom; area fill + dashed line for `military_obstacle_course` at detail zoom. Verify: `--analyze` shows Way rows; no military type in "Way types without style".
- [x] 3.4 Add very-close-zoom node icons: dedicated symbols for checkpoint/cannon/radar/embrasure/shelter/police/trench, generic `military` symbol for training_area/nuclear_explosion_site/ammunition. Verify: `--analyze` shows Node rows; no military type in "Node types without style".

## 4. Validation

- [x] 4.1 Run `build/Tests/OSTAndOSSTest stylesheets/map.ost stylesheets/standard.oss` and confirm OST/OSS load OK with no new warnings. Verify: exit 0; warning count unchanged from baseline (16 pre-existing).
- [x] 4.2 Run the same check for `cycle.oss` and `winter-sports.oss` (the other consumers of `include/military`). Verify: both load OK.
- [x] 4.3 Run `OSTAndOSSTest --analyze` and confirm every new military type has a style at some magnification and none appear in the "without style" lists. Verify: grep output shows all 15 new types with Area/Way/Node rows.
- [x] 4.4 Confirm no C++/build changes required (stylesheets only). Verify: `git diff --stat` shows only `stylesheets/map.ost` and `stylesheets/include/military.oss`.
