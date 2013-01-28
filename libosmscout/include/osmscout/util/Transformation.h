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

#include <osmscout/GeoCoord.h>

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
    enum OptimizeMethod
    {
      none = 0,
      fast = 1,
      quality = 2
    };

    TransPoint* points;

  private:
    void TransformGeoToPixel(const Projection& projection,
                             const std::vector<GeoCoord>& nodes);
    void DropSimilarPoints(double optimizeErrorTolerance);
    void DropRedundantPointsFast(double optimizeErrorTolerance);
    void DropRedundantPointsDouglasPeucker(double optimizeErrorTolerance, bool isArea);

  public:
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
                       OptimizeMethod optimize,
                       const std::vector<GeoCoord>& nodes,
                       double optimizeErrorTolerance);

    void TransformWay(const Projection& projection,
                      OptimizeMethod optimize,
                      const std::vector<GeoCoord>& nodes,
                      double optimizeErrorTolerance);

    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;

    bool GetCenterPixel(double& cx,
                        double& cy) const;
  };

  class OSMSCOUT_API TransBuffer
  {
  public:
    struct OSMSCOUT_API Pixel
    {
      double x;
      double y;

      inline Pixel()
      {
        // no code
      }

      inline Pixel (double x, double y)
      : x(x),
        y(y)
      {
        // no code
      }
    };

  private:
    TransPolygon transPolygon;
    size_t       bufferSize;
    size_t       usedPoints;

  public:
    Pixel*       buffer;

  private:
    void AssureRoomForPoints(size_t count);

  public:
    TransBuffer();
    virtual ~TransBuffer();

    void Reset();

    void TransformArea(const Projection& projection,
                       TransPolygon::OptimizeMethod optimize,
                       const std::vector<GeoCoord>& nodes,
                       size_t& start, size_t &end,
                       double optimizeErrorTolerance);
    bool TransformWay(const Projection& projection,
                      TransPolygon::OptimizeMethod optimize,
                      const std::vector<GeoCoord>& nodes,
                      size_t& start, size_t &end,
                      double optimizeErrorTolerance);

    bool GenerateParallelWay(size_t orgStart, size_t orgEnd,
                             double offset,
                             size_t& start, size_t& end);

    void GetBoundingBox(size_t start, size_t end,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax) const;

    void GetCenterPixel(size_t start, size_t end,
                        double& cx,
                        double& cy) const;
  };
}

#endif
