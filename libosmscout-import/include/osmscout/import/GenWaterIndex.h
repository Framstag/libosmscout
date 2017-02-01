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

#include <memory>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>

#include <osmscout/GroundTile.h>

#include <osmscout/import/Import.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Generator that calculates land, water and coast tiles based on costline data
   * and the assumption that land is always left of the coast (in line direction)
   * and water is always right.
   */
  class WaterIndexGenerator CLASS_FINAL : public ImportModule
  {
  private:
    /** State of a cell */
    enum State {
      unknown = 0, //! We do not know yet
      land    = 1, //! left side of the coast => a land tile
      water   = 2, //! right side of the coast => a water tile
      coast   = 3  //! The coast itself => a coast tile
    };

    /**
     * Information about a single intersection of a coastline with a cell
     */
    struct Intersection
    {
      size_t        coastline;          //! Running number of the intersecting coastline
      size_t        prevWayPointIndex;  //! The index of the path point before the intersection
      GeoCoord      point;              //! The intersection point
      double        distanceSquare;     //! The distance^2 between the path point and the intersectionPoint
      char          direction;          //! 1 in, 0 touch, -1 out
      unsigned char borderIndex;        //! The index of the border that gets intersected [0..3]
    };

    typedef Intersection *IntersectionPtr;

    /**
     * Sort intersections in path order
     */
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
    };

    /**
     * Sort intersections close wise order(clock wise around the cell border)
     */
    struct IntersectionCWComparator
    {
      inline bool operator()(const IntersectionPtr& a, const IntersectionPtr& b) const
      {
        if (a->borderIndex==b->borderIndex) {
          switch (a->borderIndex) {
          case 0:
            return a->point.GetLon()<b->point.GetLon();
          case 1:
            return a->point.GetLat()>b->point.GetLat();
          case 2:
            return a->point.GetLon()>b->point.GetLon();
          default: /* 3 */
            return a->point.GetLat()<b->point.GetLat();
          }
        }
        else {
          return a->borderIndex<b->borderIndex;
        }
      }
    };

    /**
     * A tile bitmap/zoom level.
     */
    struct Level
    {
      // Transient

      FileOffset                 indexEntryOffset; //!< File offset of this entry on disk
      double                     cellWidth;        //!< With of an cell
      double                     cellHeight;       //!< Height of an cell
      uint32_t                   cellXCount;       //!< Number of cells in horizontal direction (with of bounding box in cells)
      uint32_t                   cellYCount;       //!< Number of cells in vertical direction (height of bounding box in cells)

      // Persistent

      bool                       hasCellData;      //!< If true, we have cell data
      uint8_t                    dataOffsetBytes;  //!< Number of bytes per entry in bitmap
      State                      defaultCellData;  //!< If hasCellData is false, this is the vaue to be returned for all cells
      FileOffset                 indexDataOffset;  //!< File offset of start cell state data on disk

      uint32_t                   cellXStart;       //!< First x-axis coordinate of cells
      uint32_t                   cellXEnd;         //!< Last x-axis coordinate cells
      uint32_t                   cellYStart;       //!< First y-axis coordinate of cells
      uint32_t                   cellYEnd;         //!< Last x-axis coordinate cells

      std::vector<unsigned char> area;             //!< Actual index data

      void SetBox(const GeoCoord& minCoord,
                  const GeoCoord& maxCoord,
                  double cellWidth, double cellHeight);

      bool IsInAbsolute(uint32_t x, uint32_t y) const;
      State GetState(uint32_t x, uint32_t y) const;
      State GetStateAbsolute(uint32_t x, uint32_t y) const;

      void SetState(uint32_t x, uint32_t y, State state);
      void SetStateAbsolute(uint32_t x, uint32_t y, State state);
    };

    /**
     * A individual coastline
     */
    struct Coast
    {
      Id                 id;
      bool               isArea;
      double             sortCriteria;
      Id                 frontNodeId;
      Id                 backNodeId;
      std::vector<Point> coast;
    };

    typedef std::shared_ptr<Coast> CoastRef;

    /**
     * Holds all generated, calculated and extracted information about an
     * individual coastline
     */
    struct CoastlineData
    {
      Id                                        id;                 //! The id of the coastline
      bool                                      isArea;             //! true,if the boundary forms an area
      bool                                      isCompletelyInCell; //! true, if the complete coastline is within one cell
      Pixel                                     cell;               //! The cell that completely contains the coastline
      std::vector<GeoCoord>                     points;             //! The points of the coastline
      std::map<Pixel,std::list<Intersection> >  cellIntersections;  //! All intersections for each cell
    };

    struct Data
    {
      std::vector<CoastlineData>         coastlines;         //! data for each coastline
      std::map<Pixel,std::list<size_t> > cellCoastlines;     //! Contains for each cell the list of coastlines
    };

  private:
    std::string StateToString(State state) const;

    GroundTile::Coord Transform(const GeoCoord& point,
                                const Level& level,
                                double cellMinLat,
                                double cellMinLon,
                                bool coast);

    bool LoadCoastlines(const ImportParameter& parameter,
                        Progress& progress,
                        std::list<CoastRef>& coastlines);

    void MergeCoastlines(Progress& progress,
                         std::list<CoastRef>& coastlines);

    void MarkCoastlineCells(Progress& progress,
                            Level& level,
                            const Data& data);

    void CalculateCoastEnvironment(Progress& progress,
                                   Level& level,
                                   const std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap);

    void GetCells(const Level& level,
                  const GeoCoord& a,
                  const GeoCoord& b,
                  std::set<Pixel>& cellIntersections);

    void GetCells(const Level& level,
                  const std::vector<GeoCoord>& points,
                  std::set<Pixel>& cellIntersections);

    void GetCells(const Level& level,
                  const std::vector<Point>& points,
                  std::set<Pixel>& cellIntersections);

    void GetCellIntersections(const Level& level,
                              const std::vector<GeoCoord>& points,
                              size_t coastline,
                              std::map<Pixel,std::list<Intersection> >& cellIntersections);

    void GetCoastlineData(const ImportParameter& parameter,
                          Progress& progress,
                          Projection& projection,
                          const Level& level,
                          const std::list<CoastRef>& coastlines,
                          Data& data);

    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    Level& level);

    void FillWater(Progress& progress,
                   Level& level,
                   size_t tileCount);

    void FillLand(Progress& progress,
                  Level& level);

    void DumpIndexHeader(const ImportParameter& parameter,
                         FileWriter& writer,
                         std::vector<Level>&  levels);

    void HandleAreaCoastlinesCompletelyInACell(Progress& progress,
                                               const Level& level,
                                               Data& data,
                                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap);

    IntersectionPtr GetPreviousIntersection(std::list<IntersectionPtr>& intersectionsPathOrder,
                                            const IntersectionPtr& current);

    void WalkBorderCW(GroundTile& groundTile,
                      const Level& level,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionPtr& incoming,
                      const IntersectionPtr& outgoing,
                      const GroundTile::Coord borderCoords[]);

    IntersectionPtr GetNextCW(const std::list<IntersectionPtr>& intersectionsCW,
                              const IntersectionPtr& current) const;

    void WalkPathBack(GroundTile& groundTile,
                      const Level& level,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionPtr& outgoing,
                      const IntersectionPtr& incoming,
                      const std::vector<GeoCoord>& points,
                      bool isArea);

    void HandleCoastlinesPartiallyInACell(Progress& progress,
                                          const std::list<CoastRef>& coastlines,
                                          const Level& level,
                                          std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                          Data& data);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
