#ifndef OSMSCOUT_GENWATERINDEX_H
#define OSMSCOUT_GENWATERINDEX_H

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

#include <osmscout/Import.h>
#include <osmscout/Point.h>

namespace osmscout {

  class WaterIndexGenerator : public ImportModule
  {
  private:
    enum State {
      unknown = 0,
      land    = 1, // left side of the coast
      water   = 2, // right side of the coast
      coast   = 3
    };

    struct Area
    {
      size_t                     cellXCount;
      size_t                     cellYCount;
      std::vector<unsigned char> area;

      void SetSize(uint32_t cellXCount, uint32_t cellYCount);

      State GetState(uint32_t x, uint32_t y) const;
      void SetState(uint32_t x, uint32_t y, State state);
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
    double           cellWidth;
    double           cellHeight;
    uint32_t         cellXStart;
    uint32_t         cellXEnd;
    uint32_t         cellYStart;
    uint32_t         cellYEnd;
    uint32_t         cellXCount;
    uint32_t         cellYCount;

    double           minLon;
    double           minLat;
    double           maxLon;
    double           maxLat;

    std::list<Coast> coastlines;
    Area             area;

  private:
    void SetCoastlineCells();
    void ScanCellsHorizontally();
    void ScanCellsVertically();
    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig);
    void FillLand();
    void FillWater();

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
