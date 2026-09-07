/*
 * search_scope.h — pure helpers for the sibling-region search scope decision
 * (NaviVeylin change "regional-search").
 *
 * The expansion rule: when a default admin region is in effect, the search
 * scope may widen from that region to its sibling subregions (children of the
 * parent), bounded by a maximum region level so search data and result volume
 * stay manageable. The decision is a pure function of the parent's level and
 * the cap; the database-bound parts (loading objects, enumerating children)
 * live in OSMScoutClient.cpp.
 *
 * Kept dependency-free on purpose (no libosmscout includes) so it can be
 * unit-tested on the host without database/file I/O.
 */
#ifndef NAVIVEYLIN_SEARCH_SCOPE_H
#define NAVIVEYLIN_SEARCH_SCOPE_H

#include <cstddef>
#include <cstdint>

namespace naviveylin {

// Default maximum admin region level for sibling expansion, on the OSM
// admin_level scale: 2=country, 4=state, 5=Regierungsbezirk/district,
// 6=county, 8=municipality, 10=suburb. Expansion never crosses into scopes
// coarser than this. Level 5 covers the common "one up one down" case: a
// kreisfreie Stadt (level 6, e.g. Dortmund) and its surrounding towns (e.g.
// Bergkamen under Kreis Unna) share a Regierungsbezirk parent.
inline constexpr uint8_t kMaxSearchRegionLevel = 5;

// Normalizes a hierarchy depth (root=1, country=2, state=3, county=4, city=5,
// suburb=6) to the admin_level scale (root=0, country=2, state=4, county=6,
// city=8, suburb=10) so both level sources compare against one cap. Returns 0
// for depth 0 (defensive; a real chain always includes the region itself).
inline uint8_t NormalizeDepthToAdminLevel(size_t depth)
{
  return depth == 0 ? 0 : static_cast<uint8_t>((depth - 1) * 2);
}

// Decides whether the search scope may expand to the parent's subregions.
// Returns false when the parent level is unknown (0) or coarser than the cap
// (level < maxLevel); true when the parent is at or finer than the cap.
inline bool ShouldExpandScope(uint8_t parentLevel, uint8_t maxLevel)
{
  return parentLevel != 0 && parentLevel >= maxLevel;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_SEARCH_SCOPE_H
