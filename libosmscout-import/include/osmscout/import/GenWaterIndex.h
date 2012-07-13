#ifndef OSMSCOUT_IMPORT_GENWATERINDEX_H
#define OSMSCOUT_IMPORT_GENWATERINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <vector>

#include <osmscout/GroundTile.h>
#include <osmscout/Point.h>

#include <osmscout/import/Import.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class WaterIndexGenerator : public ImportModule
  {
  private:
    enum State {
      unknown = 0, //! We do not know yet
      land    = 1, //! left side of the coast => a land tile
      water   = 2, //! right side of the coast => a water tile
      coast   = 3  //! The coast itself => a coast tile
    };

    struct Intersection
    {
      Point  point;              //! The intersection point
      size_t prevWayPointIndex;  //! The index of the path point before the intersection
      int    direction;          //! 1 in, 0 touch, -1 out
      size_t borderIndex;        //! The index of the border that gets intersected
      double distanceSquare;     //! The distance^2 between the path point and the intersectionPoint
    };

    typedef std::list<Intersection>::iterator IntersectionPtr;

    struct IntersectionByPathComparator
    {
      inline bool operator()(const IntersectionPtr& a, const IntersectionPtr& b) const
      {
        if (a->prevWayPointIndex==b->prevWayPointIndex) {
          return a->distanceSquare<b->distanceSquare;
        }
        else {
          return a->prevWayPointIndex<b->prevWayPointIndex;
        }
      }

      inline bool operator()(const Intersection& a, const Intersection& b) const
      {
        if (a.prevWayPointIndex==b.prevWayPointIndex) {
          return a.distanceSquare<b.distanceSquare;
        }
        else {
          return a.prevWayPointIndex<b.prevWayPointIndex;
        }
      }
    };

    struct IntersectionCCWComparator
    {
      inline bool operator()(const IntersectionPtr& a, const IntersectionPtr& b) const
      {
        if (a->borderIndex==b->borderIndex) {
          switch (a->borderIndex) {
          case 0:
            return a->point.GetLon()>b->point.GetLon();
          case 1:
            return a->point.GetLat()<b->point.GetLat();
          case 2:
            return a->point.GetLon()<b->point.GetLon();
          default: /* 3 */
            return a->point.GetLat()>b->point.GetLat();
          }
        }
        else {
          return a->borderIndex<b->borderIndex;
        }
      }
    };

    struct Level
    {
      FileOffset                 indexEntryOffset;

      double                     minLat;
      double                     maxLat;
      double                     minLon;
      double                     maxLon;

      double                     cellWidth;
      double                     cellHeight;

      uint32_t                   cellXStart;
      uint32_t                   cellXEnd;
      uint32_t                   cellYStart;
      uint32_t                   cellYEnd;

      uint32_t                   cellXCount;
      uint32_t                   cellYCount;
      std::vector<unsigned char> area;

      void SetBox(uint32_t minLat, uint32_t maxLat,
                  uint32_t minLon, uint32_t maxLon,
                  double cellWidth, double cellHeight);

      bool IsIn(uint32_t x, uint32_t y) const;
      State GetState(uint32_t x, uint32_t y) const;

      void SetState(uint32_t x, uint32_t y, State state);
      void SetStateAbsolute(uint32_t x, uint32_t y, State state);
    };

    struct Coast : public Referencable
    {
      Id                 id;
      double             sortCriteria;
      std::vector<Point> coast;
    };

    typedef Ref<Coast> CoastRef;

    struct Line
    {
      double sortCriteria1;
      double sortCriteria2;
      Point  a;
      Point  b;
    };

    struct CoastlineSort : public std::binary_function<Coast, Coast, bool>
    {
      bool operator()(const Coast& a, const Coast& b) const
      {
        return a.sortCriteria<b.sortCriteria;
      }
    };

    struct SmallerLineSort : public std::binary_function<Line, Line, bool>
    {
      bool operator()(const Line& a, const Line& b) const
      {
        if (a.sortCriteria1==b.sortCriteria1) {
          return a.sortCriteria2>b.sortCriteria2;
        }
        else {
          return a.sortCriteria1<b.sortCriteria1;
        }
      }
    };

    struct BiggerLineSort : public std::binary_function<Line, Line, bool>
    {
      bool operator()(const Line& a, const Line& b) const
      {
        if (a.sortCriteria1==b.sortCriteria1) {
          return a.sortCriteria2>b.sortCriteria2;
        }
        else {
          return a.sortCriteria1>b.sortCriteria1;
        }
      }
    };

  private:
    std::list<CoastRef> coastlines;

  private:
    bool LoadCoastlines(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig);
    void MergeCoastlines(Progress& progress);
    void MarkCoastlineCells(Progress& progress,
                            Level& level);
    void ScanCellsHorizontally(Progress& progress,
                               Level& level);
    void ScanCellsVertically(Progress& progress,
                             Level& level);
    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    Level& level);
    void FillLand(Progress& progress,
                  Level& level);
    void FillWater(Progress& progress,
                   Level& level,
                   size_t tileCount);
    void DumpIndexHeader(const ImportParameter& parameter,
                         FileWriter& writer,
                         std::vector<Level>&  levels);
    void HandleAreaCoastlinesCompletelyInACell(const ImportParameter& parameter,
                                               Progress& progress,
                                               Projection& projection,
                                               const Level& level,
                                               const std::list<CoastRef>& coastlines,
                                               std::map<Coord,std::list<GroundTile> >& cellGroundTileMap);
    void GetCellIntersections(const Level& level,
                              const std::vector<Point>& points,
                              std::map<Coord,std::list<Intersection> >& cellIntersections);
    void CloseSling(GroundTile& groundTile,
                    const IntersectionPtr& incoming,
                    const IntersectionPtr& outgoing,
                    const Point borderPoints[]);
    void HandleCoastlinesPartiallyInACell(const ImportParameter& parameter,
                                          Progress& progress,
                                          Projection& projection,
                                          const Level& level,
                                          const std::list<CoastRef>& coastlines,
                                          std::map<Coord,std::list<GroundTile> >& cellGroundTileMap);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
