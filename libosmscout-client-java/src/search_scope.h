/*
 * search_scope.h — pure helpers for the sibling-region search scope decision
 * (NaviVeylin change "regional-search") and for the geographic extent that a
 * scoped search admits.
 *
 * The expansion rule: when a default admin region is in effect, the search
 * scope may widen from that region to its sibling subregions (children of the
 * parent), bounded by a maximum region level so search data and result volume
 * stay manageable. The decision is a pure function of the parent's level and
 * the cap; the database-bound parts (loading objects, enumerating children)
 * live in OSMScoutClient.cpp.
 *
 * The extent rule: a region carries no coordinates of its own, so the search
 * derives a box from the object that represents it and admits only positions
 * inside that box, in every loaded database. The box is a superset of the
 * region, so it can admit a position the region does not contain, but it never
 * drops one the region does contain. The box decision is a pure function of
 * the box and the position; loading the region's object stays in
 * OSMScoutClient.cpp.
 *
 * Kept dependency-free on purpose (no libosmscout includes) so it can be
 * unit-tested on the host without database/file I/O.
 */
#ifndef NAVIVEYLIN_SEARCH_SCOPE_H
#define NAVIVEYLIN_SEARCH_SCOPE_H

#include <cmath>
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

// Geographic extent of a resolved search scope

// A latitude/longitude box for the search-scope filter. `isSet == false` means
// "no extent available": the filter then admits every position, because a scope
// whose extent cannot be established must degrade to the unscoped behaviour
// instead of returning nothing.
struct GeoBox
{
  double minLat = 0.0;
  double minLon = 0.0;
  double maxLat = 0.0;
  double maxLon = 0.0;
  bool   isSet  = false;
};

// Half-size of the extent used for a region that is represented by a single
// node rather than by an area or a way: such a region has no outline, so its
// extent is approximated by a box around the point. 0.25 deg is about 28 km of
// latitude — wider than a city, narrower than a district.
inline constexpr double kNodeRegionFallbackDegrees = 0.25;

// An extent that filters nothing (see GeoBox::isSet).
inline GeoBox UnsetGeoBox()
{
  return GeoBox{};
}

// Builds a set extent from two corners, normalizing the corner order so a
// caller may pass them in any arrangement, and clamping the latitude to its
// valid range. A box crossing the antimeridian is not supported (no installed
// data needs one), so the longitude is kept as passed.
inline GeoBox BoxFromCorners(double lat1, double lon1, double lat2, double lon2)
{
  GeoBox box;

  box.minLat = lat1 < lat2 ? lat1 : lat2;
  box.maxLat = lat1 < lat2 ? lat2 : lat1;
  box.minLon = lon1 < lon2 ? lon1 : lon2;
  box.maxLon = lon1 < lon2 ? lon2 : lon1;

  if (box.minLat < -90.0) {
    box.minLat = -90.0;
  }
  if (box.maxLat > 90.0) {
    box.maxLat = 90.0;
  }

  box.isSet = true;

  return box;
}

// The extent of a region known only by a point (see kNodeRegionFallbackDegrees).
inline GeoBox BoxAroundPoint(double lat, double lon,
                             double halfSizeDegrees = kNodeRegionFallbackDegrees)
{
  return BoxFromCorners(lat - halfSizeDegrees, lon - halfSizeDegrees,
                        lat + halfSizeDegrees, lon + halfSizeDegrees);
}

// Whether a position lies inside the extent. An unset extent admits every
// position (fail-open), and a non-finite position is never inside a set extent:
// an unusable position cannot be placed in the scope, and it is not a usable
// result either.
inline bool IsInsideGeoBox(const GeoBox &box, double lat, double lon)
{
  if (!box.isSet) {
    return true;
  }
  if (!std::isfinite(lat) || !std::isfinite(lon)) {
    return false;
  }
  return lat >= box.minLat && lat <= box.maxLat &&
         lon >= box.minLon && lon <= box.maxLon;
}

// Whether every position inside `inner` also lies inside `outer`. This is the
// property the scope filter relies on: a region's bounding box can only admit
// positions, it never drops one that lies inside the region it was derived from.
inline bool GeoBoxContains(const GeoBox &outer, const GeoBox &inner)
{
  if (!inner.isSet) {
    // An unset extent admits every position, so only another unset extent
    // contains it.
    return !outer.isSet;
  }
  if (!outer.isSet) {
    return false;
  }
  return outer.minLat <= inner.minLat && outer.maxLat >= inner.maxLat &&
         outer.minLon <= inner.minLon && outer.maxLon >= inner.maxLon;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_SEARCH_SCOPE_H
