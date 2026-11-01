/*
 * search_scope_extent.cpp — the geographic extent of a resolved search scope
 * (NaviVeylin change "client-java-search-scope-extent", design D2; extracted
 * from the JNI translation unit for issue #20).
 *
 * The header states why this is a separate unit. This file is the derivation:
 * load the region's object, take the box it spans, and report "no extent" for
 * every outcome in which the box cannot be established.
 */

#include "search_scope_extent.h"

#include <osmscout/Area.h>
#include <osmscout/Node.h>
#include <osmscout/Way.h>

#include <osmscout/db/Database.h>
#include <osmscout/log/Logger.h>

namespace naviveylin {

GeoBox DeriveScopeExtent(const osmscout::DBInstanceRef &db,
                         const osmscout::AdminRegionRef &region)
{
  if (!region) {
    return UnsetGeoBox();
  }

  auto database=db->GetDatabase();
  if (!database) {
    osmscout::log.Warn() << "search scope: no database to derive the extent of region '"
                         << region->name << "' from - filtering disabled";
    return UnsetGeoBox();
  }

  osmscout::GeoBox box;
  bool             loaded=false;

  if (region->object.GetType()==osmscout::RefType::refArea) {
    osmscout::AreaRef area;
    if (database->GetAreaByOffset(region->object.GetFileOffset(),area)) {
      box=area->GetBoundingBox();
      loaded=box.IsValid();
    }
  }
  else if (region->object.GetType()==osmscout::RefType::refWay) {
    osmscout::WayRef way;
    if (database->GetWayByOffset(region->object.GetFileOffset(),way)) {
      box=way->GetBoundingBox();
      loaded=box.IsValid();
    }
  }
  else if (region->object.GetType()==osmscout::RefType::refNode) {
    osmscout::NodeRef node;
    loaded=database->GetNodeByOffset(region->object.GetFileOffset(),node);
    if (loaded) {
      return BoxAroundPoint(node->GetCoords().GetLat(),
                            node->GetCoords().GetLon());
    }
  }

  if (loaded) {
    return BoxFromCorners(box.GetMinCoord().GetLat(),
                          box.GetMinCoord().GetLon(),
                          box.GetMaxCoord().GetLat(),
                          box.GetMaxCoord().GetLon());
  }

  // Identity in the line, never a coordinate.
  osmscout::log.Warn() << "search scope: no extent for region '" << region->name
                       << "' (object type " << static_cast<int>(region->object.GetType())
                       << ") - filtering disabled";

  return UnsetGeoBox();
}

} // namespace naviveylin
