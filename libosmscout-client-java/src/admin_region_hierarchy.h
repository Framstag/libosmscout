/*
 * admin_region_hierarchy.h — pure helper for building admin region hierarchy
 * paths from a ResolveAdminRegionHierachie result map.
 *
 * NaviVeylin change "fix-cross-db-region-hierarchy": the hierarchy path of a
 * search result must be built exclusively from the database that produced the
 * entry. FileOffset values are per-database file positions; merging
 * resolutions from multiple loaded databases into one map keyed by raw offset
 * lets coinciding offsets (e.g. Iceland + Germany/NRW installed side by side)
 * leak foreign region names into the path.
 *
 * Kept dependency-free on purpose (osmscout/location/Location.h only) so it
 * can be unit-tested on the host without database/file I/O. Upstreamable as a
 * small generic utility.
 */
#ifndef NAVIVEYLIN_ADMIN_REGION_HIERARCHY_H
#define NAVIVEYLIN_ADMIN_REGION_HIERARCHY_H

#include <map>
#include <string>

#include <osmscout/location/Location.h>

namespace naviveylin {

/**
 * Walks the admin region parent chain of `root` through `chain` (the map
 * filled by LocationService::ResolveAdminRegionHierachie for the SAME
 * database that owns `root`) and returns the hierarchy path in root→parent
 * order (e.g. "Bergkamen/Kreis Unna/Arnsberg/Nordrhein-Westfalen/Deutschland").
 *
 * Stops at the first parent offset missing from `chain`; the result then
 * contains the resolvable prefix only. Returns an empty string for a null
 * root.
 *
 * Callers MUST only pass a chain produced by the database that owns `root` —
 * FileOffset values are meaningless across databases.
 */
inline std::string BuildAdminRegionHierarchyPath(
    const osmscout::AdminRegionRef &root,
    const std::map<osmscout::FileOffset, osmscout::AdminRegionRef> &chain)
{
  if (!root) {
    return "";
  }

  std::string path = root->name;
  osmscout::FileOffset parentOffset = root->parentRegionOffset;

  while (parentOffset != 0) {
    const auto it = chain.find(parentOffset);
    if (it == chain.end()) {
      break;
    }
    path += "/";
    path += it->second->name;
    parentOffset = it->second->parentRegionOffset;
  }

  return path;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ADMIN_REGION_HIERARCHY_H
