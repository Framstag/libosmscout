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

#include <osmscout/Point.h>

#include <osmscout/import/Import.h>

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

      void SetBox(double minLat, double maxLat,
                  double minLon, double maxLon,
                  double cellWidth, double cellHeight);

      bool IsIn(uint32_t x, uint32_t y) const;
      State GetState(uint32_t x, uint32_t y) const;

      void SetState(uint32_t x, uint32_t y, State state);
      void SetStateAbsolute(uint32_t x, uint32_t y, State state);
    };

    struct Coast
    {
      double             sortCriteria;
      std::vector<Point> coast;
    };

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
    std::list<Coast>   coastlines;
    std::vector<Level> levels;

  private:
    void SetCoastlineCells(Level& level);
    void ScanCellsHorizontally(Level& level);
    void ScanCellsVertically(Level& level);
    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    Level& level);
    void FillLand(Level& level);
    void FillWater(Level& level, size_t tileCount);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
