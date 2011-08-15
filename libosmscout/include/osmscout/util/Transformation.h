#ifndef OSMSCOUT_UTIL_TRANSFORMATION_H
#define OSMSCOUT_UTIL_TRANSFORMATION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/Point.h>
#include <osmscout/util/Projection.h>

namespace osmscout {

  struct OSMSCOUT_API TransPoint
  {
    bool   draw;
    double x;
    double y;
  };

  class OSMSCOUT_API TransPolygon
  {
  private:
    size_t      pointsSize;
    size_t      length;
    size_t      start;
    size_t      end;

  public:
    TransPoint* points;

    TransPolygon();
    virtual ~TransPolygon();

    inline bool IsEmpty() const
    {
      return length==0;
    }

    inline size_t GetLength() const
    {
      return length;
    }

    inline size_t GetStart() const
    {
      return start;
    }

    inline size_t GetEnd() const
    {
      return end;
    }
    void TransformArea(const Projection& projection,
                       bool optimize,
                       const std::vector<Point>& nodes);
    void TransformWay(const Projection& projection,
                      bool optimize,
                      const std::vector<Point>& nodes);

    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;

    bool GetCenterPixel(double& cx,
                        double& cy) const;
  };
}

#endif
