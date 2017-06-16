#ifndef OSMSCOUT_IMPORT_WATERINDEXPROCESSOR_H
#define OSMSCOUT_IMPORT_WATERINDEXPROCESSOR_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <fstream>
#include <list>
#include <memory>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Coord.h>
#include <osmscout/Pixel.h>

#include <osmscout/GroundTile.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/private/ImportImportExport.h>

namespace osmscout {

  /**
    * Just for debug. It exports path to gpx file that may be
    * open in external editor (josm for example). It is very useful
    * to visual check that path is generated/cutted/walk (...) correctly.
    *
    * Just call this method from some place, add breakpoint after it
    * and check the result...
    */
  template<typename InputIterator>
  void WriteGpx(InputIterator begin, InputIterator end, const std::string name)
  {
    std::ofstream gpxFile;

    gpxFile.open(name.c_str());
    gpxFile.imbue(std::locale("C"));
    gpxFile.precision(100);

    gpxFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    gpxFile << "<gpx creator=\"osmscout\" version=\"1.1\" xmlns=\"http://www.topografix.com/GPX/1/1\"";
    gpxFile << " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
    gpxFile << " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n";

    gpxFile << " <trk><trkseg>\n";
    for (InputIterator it=begin; it!=end;it++) {
      gpxFile << "<trkpt lat=\"" << it->GetLat() << "\" lon=\"" << it->GetLon() << "\"></trkpt>\n";
    }
    gpxFile << " </trkseg></trk>\n</gpx>";

    gpxFile.close();
  }

  extern void WriteGpx(const std::vector<Point> &path, const std::string& name);

  /**
   * Generator that calculates land, water and coast tiles based on passed costline data.
   * For more details how index is stored look to `WaterIndex` and description of tiles (cells)
   * look to `GroundTile` struct.
   *
   * For OSM data is usual that land is always left of the coast (in line direction)
   * and water is always right. But this generator don't require it, just Coast rigth/left
   * state have to be defined correctly.
   *
   * See GenWaterIndex for usage. It is expected that methods are
   * executed in this order:
   *
   *  - MergeCoastlines - merge short coastline ways to bigger one and create areas if possible
   *  - SynthesizeCoastlines - cut coastlines by data polygon
   *  - DumpIndexHeader - write index file header
   *
   *  - then for each index level:
   *    -- CalculateCoastlineData - Collects, calculates and generates
   *              a number of data about a coastline
   *    -- MarkCoastlineCells - Mark cells that intersect a coastline as coast
   *    -- HandleCoastlinesPartiallyInACell - Fills coords information for cells that intersect a coastline
   *    -- HandleAreaCoastlinesCompletelyInACell - Fills coords information for cells that completely contain a coastline
   *    -- CalculateCoastEnvironment - Calculate the cell type for cells directly around coast cells
   *    -- FillWater - Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
   *    -- FillWaterAroundIsland - fill water (bottom layer) on tiles that fully contains some island(s)
   *    -- FillLand - Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
   *    -- CalculateHasCellData - lookup if level contains some cells, setup `level.hasCellData` in such case
   *    -- WriteTiles - write data to index file
   */
  class OSMSCOUT_IMPORT_API WaterIndexProcessor CLASS_FINAL
  {
  public:
    /**
     * State that defines area type left or right from the Coast
     * - for area Coast define inner and outer type
     */
    enum class CoastState : uint8_t
    {
      undefined = 0, //! We do not know yet
      land      = 1, //! land
      water     = 2, //! water
      unknown   = 3, //! unknown
    };

    /**
     * A individual coastline
     *
     * There may be two kind of "coastlines":
     *  - classic coastlines: define border between land and water (see)
     *  - data polygon: define area where database (and classic coastlines) are defined.
     *     It usually have right/left state unknown/udefined
     */
    struct OSMSCOUT_IMPORT_API Coast
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
     * State of a cell
     */
    enum State : uint8_t
    {
      unknown = 0, //! We do not know yet
      land    = 1, //! left side of the coast => a land tile
      water   = 2, //! right side of the coast => a water tile
      coast   = 3  //! The coast itself => a coast tile
    };

    /**
     * State of Level
     */
    class OSMSCOUT_IMPORT_API StateMap
    {
    private:
      double               cellWidth;  //!< With of an cell
      double               cellHeight; //!< Height of an cell
      uint32_t             cellXStart; //!< First x-axis coordinate of cells
      uint32_t             cellXEnd;   //!< Last x-axis coordinate cells
      uint32_t             cellYStart; //!< First y-axis coordinate of cells
      uint32_t             cellYEnd;   //!< Last x-axis coordinate cells
      uint32_t             cellXCount; //!< Number of cells in horizontal direction (with of bounding box in cells)
      uint32_t             cellYCount; //!< Number of cells in vertical direction (height of bounding box in cells)
      std::vector<uint8_t> area;       //!< Actual index data

    public:
      void SetBox(const GeoBox& boundingBox,
                  double cellWidth,
                  double cellHeight);

      inline double GetCellWidth() const
      {
        return cellWidth;
      }

      inline double GetCellHeight() const
      {
        return cellHeight;
      }

      inline uint32_t GetXStart() const
      {
        return cellXStart;
      }

      inline uint32_t GetYStart() const
      {
        return cellYStart;
      }

      inline uint32_t GetXEnd() const
      {
        return cellXEnd;
      }

      inline uint32_t GetYEnd() const
      {
        return cellYEnd;
      }

      inline uint32_t GetXCount() const
      {
        return cellXCount;
      }

      inline uint32_t GetYCount() const
      {
        return cellYCount;
      }

      inline bool IsInAbsolute(uint32_t x, uint32_t y) const
      {
        return x>=cellXStart &&
               x<=cellXEnd &&
               y>=cellYStart &&
               y<=cellYEnd;
      }

      State GetState(uint32_t x, uint32_t y) const;
      inline State GetStateAbsolute(uint32_t x, uint32_t y) const
      {
        return GetState(x-cellXStart,
                        y-cellYStart);
      }

      void SetState(uint32_t x, uint32_t y, State state);
      inline void SetStateAbsolute(uint32_t x, uint32_t y, State state)
      {
        SetState(x-cellXStart,
                 y-cellYStart,
                 state);
      }
    };

    /**
     * A tile bitmap/zoom level.
     */
    struct OSMSCOUT_IMPORT_API Level
    {
      // Transient
      uint32_t             level;            //!< The actual zoom level
      FileOffset           indexEntryOffset; //!< File offset of this entry on disk

      // Persistent
      bool                 hasCellData;      //!< If true, we have cell data
      uint8_t              dataOffsetBytes;  //!< Number of bytes per entry in bitmap
      State                defaultCellData;  //!< If hasCellData is false, this is the vaue to be returned for all cells
      FileOffset           indexDataOffset;  //!< File offset of start cell state data on disk

      StateMap             stateMap;         //!< Index to handle state of cells

      void SetBox(const GeoBox& boundingBox,
                  double cellWidth,
                  double cellHeight);
    };

    /**
     * Direction of Intersection (index cell and coastline)
     */
    enum Direction : int8_t
    {
      out   = -1,
      touch = 0,
      in    = 1
    };

    /**
     * Information about a single intersection of a coastline with a cell
     */
    struct OSMSCOUT_IMPORT_API Intersection
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
     * Holds all generated, calculated and extracted information about an
     * individual coastline
     */
    struct OSMSCOUT_IMPORT_API CoastlineData
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

    /**
     * Computed data for one index level
     */
    struct OSMSCOUT_IMPORT_API Data
    {
      std::vector<CoastlineDataRef>     coastlines;            //! data for each coastline
      std::map<Pixel,std::list<size_t>> cellCoastlines;        //! Contains for each cell the list of *intersecting* coastlines
      std::map<Pixel,std::list<size_t>> cellCoveredCoastlines; //! Contains for each cell the list of covered coastlines
    };

  private:

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

      inline CellBoundaries(const StateMap &stateMap, const Pixel &cell)
      {
        lonMin=(stateMap.GetXStart()+cell.x)*stateMap.GetCellWidth()-180.0;
        lonMax=(stateMap.GetXStart()+cell.x+1)*stateMap.GetCellWidth()-180.0;
        latMin=(stateMap.GetYStart()+cell.y)*stateMap.GetCellHeight()-90.0;
        latMax=(stateMap.GetYStart()+cell.y+1)*stateMap.GetCellHeight()-90.0;

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

  private:
    std::string StateToString(State state) const;
    std::string TypeToString(GroundTile::Type type) const;

    /**
     * Transform the given geo coordinate to a corresponding GroundTile coordinate
     *
     * @param point
     *    geo coordinate to transform
     * @param stateMap
     *    the StateMap, holding information about cell dimensions
     * @param cellMinLat
     *    offset of the ground map
     * @param cellMinLon
     *    offset of the ground map
     * @param coast
     *    Flag to be copied into the GroundTile coordinate
     * @return
     */
    GroundTile::Coord Transform(const GeoCoord& point,
                                const StateMap& stateMap,
                                double cellMinLat,
                                double cellMinLon,
                                bool coast);

    /**
     * Return the list of StateMap cells a line between coordinate a and coordinate b
     * intersects with.
     *
     * @param stateMap
     *    StateMap defining the cell structure
     * @param a
     *    Start of the line
     * @param b
     *    End of the line
     * @param cellIntersections
     *    List of cells, the line intersects with
     */
    void GetCells(const StateMap& stateMap,
                  const GeoCoord& a,
                  const GeoCoord& b,
                  std::set<Pixel>& cellIntersections) const;

    /**
     * Compute all cell intersections for specific coastline (with index `coastline`, and path `points`)
     * with index cells. Computed intesections are stored to `cellIntersections` map.
     *
     * @param stateMap - state of index level
     * @param points - coastline way
     * @param coastline - index of coastline
     * @param cellIntersections - map of all intersections for specific cell
     */
    void GetCellIntersections(const StateMap& stateMap,
                              const std::vector<GeoCoord>& points,
                              size_t coastline,
                              std::map<Pixel,std::list<IntersectionRef>>& cellIntersections);

    /**
     * Return true if cell defined by `cellBoundary` at least partly belongs in some bounding polygon.
     */
    bool IsCellInBoundingPolygon(const CellBoundaries& cellBoundary,
                                 const std::list<CoastRef>& boundingPolygons);

    /**
     * Return true if some tile from `tiles` contains `coord` with `type`
     */
    bool ContainsCoord(const std::list<GroundTile> &tiles,
                       const GroundTile::Coord &coord,
                       GroundTile::Type type);

    /**
     * Return true if some tile from `tiles` contains `coord`
     */
    bool ContainsCoord(const std::list<GroundTile> &tiles,
                       const GroundTile::Coord &coord);

    /**
     * Return true if cell `coord` contains water (its state is water or contains water coord)
     */
    bool ContainsWater(const Pixel &coord,
                       const StateMap &stateMap,
                       const std::map<Pixel,std::list<GroundTile>>& cellGroundTileMap,
                       const GroundTile::Coord &testCoord1,
                       const GroundTile::Coord &testCoord2);

    /**
     * Closes the sling from the `incoming` intersection to the `outgoing` intersection 
     * traveling clock wise around the cell border.
     * Walk points are stored to given groundTile.
     */
    void WalkBorderCW(GroundTile& groundTile,
                      const StateMap& stateMap,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionRef& incoming,
                      const IntersectionRef& outgoing,
                      const GroundTile::Coord borderCoords[]);

    /**
     * Return next clock wise intersection (from `current`)
     */
    IntersectionRef GetNextCW(const std::list<IntersectionRef>& intersectionsCW,
                              const IntersectionRef& current) const;

    /**
     * Walk path/area (defined by `points`, `isArea`) back from `pathStart` to `pathEnd`
     * and store walked points to given groundTile
     */
    void WalkPathBack(GroundTile& groundTile,
                      const StateMap& stateMap,
                      double cellMinLat,
                      double cellMinLon,
                      const IntersectionRef& pathStart,
                      const IntersectionRef& pathEnd,
                      const std::vector<GeoCoord>& points,
                      bool isArea);

    /**
     * Walk path/area (defined by `points`, `isArea`) from `pathStart` to `pathEnd`
     * and store walked points to given groundTile
     */
    void WalkPathForward(GroundTile& groundTile,
                         const StateMap& stateMap,
                         double cellMinLat,
                         double cellMinLon,
                         const IntersectionRef& pathStart,
                         const IntersectionRef& pathEnd,
                         const std::vector<GeoCoord>& points,
                         bool isArea);

    /**
     * Return following (or previous) intersection for same coastline.
     *
     * If intersection->direction==Direction::in, we are searching next OUT intersection,
     * previos IN intersection othervise.
     */
    IntersectionRef FindSiblingIntersection(const IntersectionRef &intersection,
                                            const std::list<IntersectionRef> &intersectionsCW,
                                            bool isArea);

    /**
     * Walk from "tripoint" in `pathEnd` via "next" coastline.
     * Walked points are stored to given groundTile.
     *
     * Tripoing is point where three different coastlines are met:
     *  - water / land
     *  - land / unknown
     *  - unkwnon / water
     * It can't be merged in such case (different types).
     */
    bool WalkFromTripoint(GroundTile &groundTile,
                          const StateMap& stateMap,
                          const CellBoundaries &cellBoundaries,
                          IntersectionRef &pathStart,
                          IntersectionRef &pathEnd,
                          Data &data,
                          const std::list<IntersectionRef> &intersectionsCW,
                          const std::vector<size_t> &containingPaths);

    /**
     * Walk path/area (defined by `points`, `isArea`) from `pathStart` to `pathEnd`
     * and store walked points to given groundTile
     */
    void WalkPath(GroundTile &groundTile,
                  const StateMap& stateMap,
                  const CellBoundaries &cellBoundaries,
                  const IntersectionRef pathStart,
                  const IntersectionRef pathEnd,
                  CoastlineDataRef coastline);

    /**
     * Walk from `outIntersection` clock wise via coastline paths and cell borders
     * back to starting point (`outIntersection`).
     * Walked points are stored to given `groundTile`.
     */
    bool WalkBoundaryCW(GroundTile &groundTile,
                        const StateMap &stateMap,
                        const IntersectionRef outIntersection,
                        const std::list<IntersectionRef> &intersectionsCW,
                        std::set<IntersectionRef> &visitedIntersections,
                        const CellBoundaries &cellBoundaries,
                        Data& data,
                        const std::vector<size_t> &containingPaths);

    /**
     * Take the given coastlines and bounding polygons and create a list of synthesized
     * coastlines that fuly encircle the imported region. Coastlines are either
     * real-life coastlines or emulated coastlines based on the bounding
     * polygons.
     *
     * @param progress
     * @param boundingPolygons
     * @param coastlines
     * @param synthesized
     */
    void SynthesizeCoastlines2(Progress& progress,
                               const std::list<CoastRef>& boundingPolygons,
                               const std::list<CoastRef>& coastlines,
                               std::list<CoastRef>& synthesized);

    /**
     * Generate all ground tiles (store to `cellGroundTileMap`) for given `cell`.
     */
    void HandleCoastlineCell(Progress& progress,
                             const Pixel &cell,
                             const std::list<size_t>& intersectCoastlines,
                             const StateMap& stateMap,
                             std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                             Data& data);

public:
    /**
     * Merge short coastline ways to bigger one and create areas if possible.
     */
    void MergeCoastlines(Progress& progress,
                         std::list<WaterIndexProcessor::CoastRef>& coastlines);

    /**
     * Return the list of StateMap cells the given path intersects with
     *
     * @param stateMap
     *    StateMap defining the cell structure
     * @param points
     *    Path
     * @param cellIntersections
     *    List of cells, the path intersects with
     */
    void GetCells(const StateMap& stateMap,
                  const std::vector<GeoCoord>& points,
                  std::set<Pixel>& cellIntersections) const;

    /**
     * Return the list of StateMap cells the given path intersects with
     *
     * @param stateMap
     *    StateMap defining the cell structure
     * @param points
     *    Path
     * @param cellIntersections
     *    List of cells, the path intersects with
     */
    void GetCells(const StateMap& stateMap,
                  const std::vector<Point>& points,
                  std::set<Pixel>& cellIntersections) const;

    /**
     * try to synthetize coastline segments from all way coastlines
     * that intersect with the bounding polygon
     *
     * @param progress
     * @param coastlines
     * @param boundingPolygons
     */
    void SynthesizeCoastlines(Progress& progress,
                            std::list<CoastRef>& coastlines,
                            std::list<CoastRef>& boundingPolygons);

    /**
     * Collects, calculates and generates a number of data about a coastline.
     */
    void CalculateCoastlineData(Progress& progress,
                                TransPolygon::OptimizeMethod optimizationMethod,
                                double tolerance,
                                double minObjectDimension,
                                const Projection& projection,
                                const StateMap& stateMap,
                                const std::list<CoastRef>& coastlines,
                                Data& data);

    /**
     * Markes a cell as "coast", if one of the coastlines intersects with it.
     */
    void MarkCoastlineCells(Progress& progress,
                            StateMap& stateMap,
                            const Data& data);

    /**
     * Fills coords information for cells that completely contain a coastline
     *
     * @param progress
     * @param level
     * @param data
     * @param cellGroundTileMap
     */
    void HandleAreaCoastlinesCompletelyInACell(Progress& progress,
                                               const StateMap& stateMap,
                                               Data& data,
                                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap);

    /**
     * Fills coords information for cells that intersect a coastline
     */
    void HandleCoastlinesPartiallyInACell(Progress& progress,
                                          const StateMap& stateMap,
                                          std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                                          Data& data);

    /**
     * Calculate the cell type for cells directly around coast cells
     * 
     * @param progress
     * @param stateMap
     * @param cellGroundTileMap
     */
    void CalculateCoastEnvironment(Progress& progress,
                                   StateMap& stateMap,
                                   const std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap);

    /**
     * fill water (bottom layer) on tiles that fully contains some island(s)
     */
    void FillWaterAroundIsland(Progress& progress,
                               StateMap& stateMap,
                               std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap,
                               const std::list<CoastRef>& boundingPolygons);

    /**
     * Marks all still 'unknown' cells neighbouring 'water' cells as 'water', too
     *
     * Converts all cells of state "unknown" that touch a tile with state
     * "water" to state "water", too.
     */
    void FillWater(Progress& progress,
                   Level& level,
                   size_t tileCount,
                   const std::list<CoastRef>& boundingPolygons);

    /**
     * Marks all still 'unknown' cells between 'coast' or 'land' and 'land' cells as 'land', too
     *
     * Scanning from left to right and bottom to top: Every tile that is unknown
     * but is placed between land and coast or land cells must be land, too.
     */
    void FillLand(Progress& progress,
                  StateMap& stateMap);

    /**
     * Lookup if level contains some cells, setup `level.hasCellData` in such case
     */
    void CalculateHasCellData(Level& level,
                              const std::map<Pixel,std::list<GroundTile> >& cellGroundTileMap) const;

    /**
     * write index file header
     */
    void DumpIndexHeader(FileWriter& writer,
                         std::vector<Level>& levels);

    /**
     * write data to index file
     */
    void WriteTiles(Progress& progress,
                    const std::map<Pixel,std::list<GroundTile>>& cellGroundTileMap,
                    Level& level,
                    FileWriter& writer);
  };
}

#endif
