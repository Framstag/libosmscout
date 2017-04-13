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
#include <osmscout/Coord.h>
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

    enum Direction {
      out   = -1,
      touch = 0,
      in    = 1
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
      Direction     direction;          //! 1 in, 0 touch, -1 out
      unsigned char borderIndex;        //! The index of the border that gets intersected [0..3]
    };

    typedef std::shared_ptr<Intersection> IntersectionRef;

    /**
     * Sort intersections in path order
     */
    struct IntersectionByPathComparator
    {
      inline bool operator()(const IntersectionRef& a, const IntersectionRef& b) const
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
      inline bool operator()(const IntersectionRef& a, const IntersectionRef& b) const
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
     * Helper for cell boundaries
     */
    struct CellBoundaries
    {
      double lonMin;
      double lonMax;
      double latMin;
      double latMax;

      GroundTile::Coord borderCoords[4];
      GeoCoord borderPoints[4];

      inline CellBoundaries(const Level &level, const Pixel &cell){
        lonMin=(level.cellXStart+cell.x)*level.cellWidth-180.0;
        lonMax=(level.cellXStart+cell.x+1)*level.cellWidth-180.0;
        latMin=(level.cellYStart+cell.y)*level.cellHeight-90.0;
        latMax=(level.cellYStart+cell.y+1)*level.cellHeight-90.0;

        borderPoints[0]=GeoCoord(latMax,lonMin); // top left
        borderPoints[1]=GeoCoord(latMax,lonMax); // top right
        borderPoints[2]=GeoCoord(latMin,lonMax); // bottom right
        borderPoints[3]=GeoCoord(latMin,lonMin); // bottom left

        borderCoords[0].Set(0,GroundTile::Coord::CELL_MAX,false);                           // top left
        borderCoords[1].Set(GroundTile::Coord::CELL_MAX,GroundTile::Coord::CELL_MAX,false); // top right
        borderCoords[2].Set(GroundTile::Coord::CELL_MAX,0,false);                           // bottom right
        borderCoords[3].Set(0,0,false);                                                     // bottom left
      }
    };

    /**
     * State that defines area type left from the Coast
     * - for area Coast define inner and outer type
     */
    enum class CoastState {
      undefined = 0, //! We do not know yet
      land      = 1, //! land
      water     = 2, //! water
      unknown   = 3, //! unknown
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
      CoastState         left;
      CoastState         right;
    };

    typedef std::shared_ptr<Coast> CoastRef;

    /**
     * Holds all generated, calculated and extracted information about an
     * individual coastline
     */
    struct CoastlineData
    {
      Id                                         id;                 //! The id of the coastline
      bool                                       isArea;             //! true,if the boundary forms an area
      bool                                       isCompletelyInCell; //! true, if the complete coastline is within one cell
      Pixel                                      cell;               //! The cell that completely contains the coastline
      std::vector<GeoCoord>                      points;             //! The points of the coastline
      std::map<Pixel,std::list<IntersectionRef>> cellIntersections;  //! All intersections for each cell
      CoastState                                 left;
      CoastState                                 right;
    };

    typedef std::shared_ptr<CoastlineData> CoastlineDataRef;

    struct Data
    {
      std::vector<CoastlineDataRef>     coastlines;            //! data for each coastline
      std::map<Pixel,std::list<size_t>> cellCoastlines;        //! Contains for each cell the list of *intersecting* coastlines
      std::map<Pixel,std::list<size_t>> cellCoveredCoastlines; //! Contains for each cell the list of covered coastlines
    };

  private:
    std::string StateToString(State state) const;
    std::string TypeToString(GroundTile::Type type) const;

    GroundTile::Coord Transform(const GeoCoord& point,
                                const Level& level,
                                double cellMinLat,
                                double cellMinLon,
                                bool coast);

    bool LoadRawBoundaries(const ImportParameter& parameter,
                           Progress& progress,
                           std::list<CoastRef>& coastlines,
                           const char* rawFile,
                           CoastState leftState,
                           CoastState rightState);

    bool LoadCoastlines(const ImportParameter& parameter,
                        Progress& progress,
                        std::list<CoastRef>& coastlines);

    bool LoadDataPolygon(const ImportParameter& parameter,
                         Progress& progress,
                         std::list<CoastRef>& coastlines);

    void SynthetizeCoastlines2(Progress& progress,
                               const std::list<CoastRef>& dataPolygons,
                               const std::list<CoastRef>& coastlines,
                               std::list<CoastRef> &synthetized);

    void MergeCoastlines(Progress& progress,
                         std::list<CoastRef>& coastlines);

    void SynthetizeCoastlines(Progress& progress,
                              std::list<CoastRef>& coastlines,
                              std::list<CoastRef>& dataPolygon);

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
                              std::map<Pixel,std::list<IntersectionRef>>& cellIntersections);

    void GetCoastlineData(const ImportParameter& parameter,
                          Progress& progress,
                          const Projection& projection,
                          const Level& level,
                          const std::list<CoastRef>& coastlines,
                          Data& data);

    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    Level& level);

    bool IsCellInDataPolygon(const CellBoundaries &cellBoundary,
                             const std::list<CoastRef>& dataPolygon);

    void FillWater(Progress& progress,
                   Level& level,
                   size_t tileCount,
                   const std::list<CoastRef>& dataPolygon);

    bool ContainsCoord(const std::list<GroundTile> &tiles,
                       const GroundTile::Coord &coord,
                       GroundTile::Type type);

    bool ContainsCoord(const std::list<GroundTile> &tiles,
                       const GroundTile::Coord &coord);

    bool ContainsWater(const Pixel &coord,
                       const Level &level,
                       const std::map<Pixel,std::list<GroundTile>>& cellGroundTileMap,
                       const GroundTile::Coord &testCoord1,
                       const GroundTile::Coord &testCoord2);

    void FillWaterAroundIsland(Progress& progress,
                               Level& level,
                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                               const std::list<CoastRef>& dataPolygon);

    void FillLand(Progress& progress,
                  Level& level);

    void DumpIndexHeader(const ImportParameter& parameter,
                         FileWriter& writer,
                         std::vector<Level>&  levels);

    void HandleAreaCoastlinesCompletelyInACell(Progress& progress,
                                               const Level& level,
                                               Data& data,
                                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap);

    IntersectionRef GetPreviousIntersection(std::list<IntersectionRef>& intersectionsPathOrder,
                                            const IntersectionRef& current);

    void WalkBorderCW(GroundTile& groundTile,
                      const Level& level,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionRef& incoming,
                      const IntersectionRef& outgoing,
                      const GroundTile::Coord borderCoords[]);

    IntersectionRef GetNextCW(const std::list<IntersectionRef>& intersectionsCW,
                              const IntersectionRef& current) const;

    void WalkPathBack(GroundTile& groundTile,
                      const Level& level,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionRef& pathStart,
                      const IntersectionRef& pathEnd,
                      const std::vector<GeoCoord>& points,
                      bool isArea);

    void WalkPathForward(GroundTile& groundTile,
                         const Level& level,
                         double cellMinLat,
                         double cellMinLon,
                         const IntersectionRef& pathStart,
                         const IntersectionRef& pathEnd,
                         const std::vector<GeoCoord>& points,
                         bool isArea);

    IntersectionRef FindSiblingIntersection(const IntersectionRef &intersection,
                                            const std::list<IntersectionRef> &intersectionsCW,
                                            bool isArea);

    bool WalkFromTripoint(GroundTile &groundTile,
                          const Level& level,
                          const CellBoundaries &cellBoundaries,
                          IntersectionRef &pathStart,
                          IntersectionRef &pathEnd,
                          Data &data,
                          const std::list<IntersectionRef> &intersectionsCW,
                          const std::vector<size_t> &containingPaths);

    void WalkPath(GroundTile &groundTile,
                  const Level& level,
                  const CellBoundaries &cellBoundaries,
                  const IntersectionRef pathStart,
                  const IntersectionRef pathEnd,
                  CoastlineDataRef coastline);

    bool WalkBoundaryCW(GroundTile &groundTile,
                          const Level &level,
                          const IntersectionRef outIntersection,
                          const std::list<IntersectionRef> &intersectionsCW,
                          std::set<IntersectionRef> &visitedIntersections,
                          const CellBoundaries &cellBoundaries,
                          Data& data,
                          const std::vector<size_t> &containingPaths);

      void HandleCoastlineCell(Progress& progress,
                               const Pixel &cell,
                               const std::list<size_t>& intersectCoastlines,
                               const Level& level,
                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                               Data& data);

      void HandleCoastlinesPartiallyInACell(Progress& progress,
                                            const Level& level,
                                            std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                            Data& data);

      void buildTiles(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress &progress,
                      const MercatorProjection &projection,
                      Level &levelStruct,
                      std::map<Pixel,std::list<GroundTile>> &cellGroundTileMap,
                      const std::list<CoastRef> &coastlines,
                      Data &data,
                      const std::list<CoastRef>& dataPolygon);

      void writeTiles(Progress &progress,
                      const std::map<Pixel,std::list<GroundTile>> &cellGroundTileMap,
                      const uint32_t level,
                      Level &levelStruct,
                      FileWriter &writer);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
