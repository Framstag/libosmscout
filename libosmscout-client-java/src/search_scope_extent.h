/*
 * search_scope_extent.h — the geographic extent of a resolved search scope,
 * derived from the object that represents the scope region (NaviVeylin change
 * "client-java-search-scope-extent", design D2).
 *
 * Why this lives beside search_scope.h and not in OSMScoutClient.cpp: the
 * decision it feeds (`naviveylin::IsInsideGeoBox`) is a pure function that
 * Tests/src/SearchScopeTest.cpp covers, but the derivation of the box — loading
 * the region's object and taking its bounding box — was reachable only from the
 * JNI translation unit, which the host build cannot compile. A regression that
 * stopped deriving a box, or that derived it from the wrong object, was
 * therefore caught by no test at all (issue #20). Nothing here touches JNI, so
 * it belongs in a unit the host suite compiles and links.
 *
 * The database and the region stay the caller's: this unit derives a box, it
 * never walks, filters or collects.
 */
#ifndef NAVIVEYLIN_SEARCH_SCOPE_EXTENT_H
#define NAVIVEYLIN_SEARCH_SCOPE_EXTENT_H

#include <osmscout/location/Location.h>

#include <osmscoutclient/DBInstance.h>

#include "search_scope.h"

namespace naviveylin {

/*
 * Derives the geographic extent of a resolved search scope from the object that
 * represents the region.
 *
 * An admin region carries no coordinates of its own: the extent therefore comes
 * from its object — an area or a way contributes its bounding box, a node region
 * only a point, which is approximated by the documented fallback box. Every
 * other outcome (no region, no database, the object not loadable, an invalid
 * box) leaves the extent unset, which makes the filter admit every position.
 * That is deliberate: a scope whose extent cannot be established must degrade to
 * the unscoped behaviour, never silently empty the search.
 */
GeoBox DeriveScopeExtent(const osmscout::DBInstanceRef &db,
                         const osmscout::AdminRegionRef &region);

} // namespace naviveylin

#endif // NAVIVEYLIN_SEARCH_SCOPE_EXTENT_H
