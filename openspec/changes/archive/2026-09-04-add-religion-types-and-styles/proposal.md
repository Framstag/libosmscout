## Why

The libosmscout stylesheets only define worship types for christian, jewish, and muslim religions. All other religions documented on the OSM wiki Key:religion page (buddhist, hindu, shinto, taoist, sikh, jain, pagan, zoroastrian, chinese_folk, multifaith, and the smaller documented values) fall through to the generic `amenity` type and are not rendered with religion-specific labels or icons. Additionally, the existing `muslim_mosque_building` type contains a typo (`"religion"=="muslin"` instead of `"muslim"`), so muslim mosques are not matched.

## What Changes

- Add NODE worship type definitions for every religion value documented on the OSM wiki Key:religion page (all values may be mapped on nodes and areas; areas are already covered by the existing generic `temple_building`, `shrine_building`, and `worship_building` types).
- Fix the `muslin` → `muslim` typo in the `muslim_mosque_building` type definition.
- Add religion-specific symbols and style definitions in `include/religious.oss` for the new worship types, reusing the existing christian worship style pattern (label + icon at very close zoom).
- Add symbols for the major religions (jewish star of David, muslim crescent, buddhist dharma wheel, taoist yin-yang, shinto torii, pagan pentagram) and a generic place-of-worship symbol for the remaining religions.
- Rename all religion symbols with a `religion_` prefix for consistent naming.
- Omit `religion=none` (discouraged irreligious use per the wiki).

## Capabilities

### New Capabilities
- `religion-type-definitions`: Type and style definitions for all OSM religion values in the libosmscout stylesheets, so that all relevant `religion`-tagged place-of-worship objects are recognized and rendered.

### Modified Capabilities
<!-- none -->

## Impact

- `stylesheets/map.ost` — new worship type definitions in the Religious section; typo fix in `muslim_mosque_building`
- `stylesheets/include/religious.oss` — new symbols and style definitions for the worship types
- `stylesheets/standard.oss`, `stylesheets/cycle.oss` — consume `include/religious` module (no direct change)
- No C++ code, build system, or API changes
