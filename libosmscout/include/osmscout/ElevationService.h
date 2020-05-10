#ifndef OSMSCOUT_ELEVATION_SERVICE_H
#define OSMSCOUT_ELEVATION_SERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2019  Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>
#include <osmscout/CoreImportExport.h>
#include <osmscout/FeatureReader.h>

#include <osmscout/util/TileId.h>

#include <vector>

namespace osmscout {

struct ContoursData
{
  EleFeatureValueReader reader;
  std::vector<WayRef> contours;
};

struct ElevationPoint
{
  Distance distance;
  Distance elevation;
  GeoCoord coord;
  WayRef contour;
};

template <typename DataLoader>
class ElevationService CLASS_FINAL
{
private:
  DataLoader &dataLoader;
  MagnificationLevel loadTileMag;

public:
  explicit ElevationService(DataLoader &dataLoader,
                            MagnificationLevel loadTileMag = Magnification::magSuburb):
      dataLoader(dataLoader), loadTileMag(loadTileMag)
  {}

  std::vector<ElevationPoint> ElevationProfile(const std::vector<GeoCoord> &way)
  {
    std::vector<ElevationPoint> result;
    Distance distance;
    GeoCoord intersection;

    std::vector<ContoursData> contours;
    GeoBox loadBox;

    for (size_t i=0; i < way.size()-1; i+=1){
      GeoCoord a1=way[i];
      GeoCoord a2=way[i+1];
      GeoBox lineBox(a1,a2);

      if (!loadBox.Includes(a1) || !loadBox.Includes(a2)) {
        TileId tile1=TileId::GetTile(loadTileMag, lineBox.GetMinCoord());
        TileId tile2=TileId::GetTile(loadTileMag, lineBox.GetMaxCoord());
        TileIdBox tileBox(tile1, tile2);
        loadBox=tileBox.GetBoundingBox(Magnification(loadTileMag));
        if (tileBox.GetCount() > 100){
          osmscout::log.Warn() << "Too huge area for loading " << loadBox.GetDisplayText();
          continue;
        }
        //osmscout::log.Debug() << "box " << lineBox.GetDisplayText() << " -> " << loadBox.GetDisplayText() << " size: " << tileBox.GetCount();
        assert(loadBox.Includes(a1));
        assert(loadBox.Includes(a2));
        contours = dataLoader.LoadContours(loadBox);
      }

      for (const ContoursData &contoursData:contours) {

        for (const auto &contour:contoursData.contours) {
          if (!lineBox.Intersects(contour->GetBoundingBox())){
            continue;
          }

          EleFeatureValue *eleValue=contoursData.reader.GetValue(contour->GetFeatureValueBuffer());
          if (eleValue==nullptr) {
            continue;
          }

          for (size_t bi=0; bi < contour->nodes.size()-1; bi+=1){
            GeoCoord b1=contour->nodes[bi].GetCoord();
            GeoCoord b2=contour->nodes[bi+1].GetCoord();
            if (GetLineIntersection(a1,a2,
                                    b1,b2,
                                    intersection)) {

              result.push_back(ElevationPoint{
                                   distance + GetEllipsoidalDistance(a1, intersection),
                                   Meters(eleValue->GetEle()),
                                   intersection,
                                   contour
                               });

              //log.Debug() << "  " << eleValue->GetEle() << " m, distance " << distance << " + " << GetEllipsoidalDistance(a1, intersection) << " : " << intersection.GetDisplayText();
            }
          }

        }
      }
      //log.Debug() << "At distance " << distance << " adding " << GetEllipsoidalDistance(a1,a2) << " (" << a1.GetDisplayText() << " -> " << a2.GetDisplayText() << ")";
      distance+=GetEllipsoidalDistance(a1,a2);
    }
    std::sort(result.begin(), result.end(), [](const ElevationPoint &a, const ElevationPoint &b) {
      if (a.distance != b.distance) {
        return a.distance < b.distance;
      }
      if (a.elevation != b.elevation) {
        return a.elevation < b.elevation;
      }
      return a.coord < b.coord;
    });
    return result;
  }
};

}

#endif //OSMSCOUT_ELEVATION_SERVICE_H
