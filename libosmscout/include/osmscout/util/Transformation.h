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

// For memcpy
#include <cstring>
#include <vector>

#include <osmscout/CoreImportExport.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Projection.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   */
  class OSMSCOUT_API TransPolygon CLASS_FINAL
  {
  private:
    size_t  pointsSize;
    size_t  length;
    size_t  start;
    size_t  end;

  public:
    enum OptimizeMethod
    {
      none = 0,
      fast = 1,
      quality = 2
    };

    enum OutputConstraint
    {
      noConstraint = 0,
      simple = 1
    };

    struct OSMSCOUT_API TransPoint
    {
      bool   draw;
      double x;
      double y;
    };

  private:
    struct TransPointRef
    {
      TransPoint *p;

      inline double GetLat() const
      {
        return p->x;
      }

      inline double GetLon() const
      {
        return p->y;
      }

      inline bool IsEqual(const TransPointRef &other) const
      {
        return p==other.p;
      }
    };

  public:
    TransPoint* points;

  private:
    void TransformGeoToPixel(const Projection& projection,
                             const std::vector<GeoCoord>& nodes);
    void TransformGeoToPixel(const Projection& projection,
                             const std::vector<Point>& nodes);
    void DropSimilarPoints(double optimizeErrorTolerance);
    void DropRedundantPointsFast(double optimizeErrorTolerance);
    void DropRedundantPointsDouglasPeucker(double optimizeErrorTolerance, bool isArea);
    void DropEqualPoints();
    void EnsureSimple(bool isArea);

  public:
    TransPolygon();
    ~TransPolygon();

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
                       double optimizeErrorTolerance,
                       OutputConstraint constraint=noConstraint);
    void TransformArea(const Projection& projection,
                       OptimizeMethod optimize,
                       const std::vector<Point>& nodes,
                       double optimizeErrorTolerance,
                       OutputConstraint constraint=noConstraint);

    void TransformWay(const Projection& projection,
                      OptimizeMethod optimize,
                      const std::vector<GeoCoord>& nodes,
                      double optimizeErrorTolerance,
                      OutputConstraint constraint=noConstraint);
    void TransformWay(const Projection& projection,
                      OptimizeMethod optimize,
                      const std::vector<Point>& nodes,
                      double optimizeErrorTolerance,
                      OutputConstraint constraint=noConstraint);

    void TransformBoundingBox(const Projection& projection,
                              OptimizeMethod optimize,
                              const GeoBox& boundingBox,
                              double optimizeErrorTolerance,
                              OutputConstraint constraint=noConstraint);

    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;
  };

  /**
   * \ingroup Geometry
   */
  class OSMSCOUT_API CoordBuffer CLASS_FINAL
  {
  private:
    size_t   bufferSize;
    size_t   usedPoints;

  public:
    Vertex2D *buffer;

  public:
    CoordBuffer();
    ~CoordBuffer();

    void Reset();
    size_t PushCoord(double x, double y);

    /**
     * Generate parallel way to way stored in this buffer on range orgStart, orgEnd (inclusive)
     * Result is stored after the last valid point. Generated way offsets are returned
     * in start and end.
     *
     * @param orgStart original way start
     * @param orgEnd original way end (inclusive)
     * @param offset offset of parallel way - positive offset is left, negative right
     * @param start start of result
     * @param end end of result (inclusive)
     * @return true on success, false othervise
     */
    bool GenerateParallelWay(size_t orgStart,
                             size_t orgEnd,
                             double offset,
                             size_t& start,
                             size_t& end);
  };


  /**
   * \ingroup Geometry
   */
  class OSMSCOUT_API TransBuffer CLASS_FINAL
  {
  public:
    TransPolygon transPolygon;
    CoordBuffer  *buffer;

  public:
    explicit TransBuffer(CoordBuffer* buffer);
    ~TransBuffer();

    void Reset();

    void TransformArea(const Projection& projection,
                       TransPolygon::OptimizeMethod optimize,
                       const std::vector<Point>& nodes,
                       size_t& start, size_t &end,
                       double optimizeErrorTolerance);
    bool TransformWay(const Projection& projection,
                      TransPolygon::OptimizeMethod optimize,
                      const std::vector<Point>& nodes,
                      size_t& start, size_t &end,
                      double optimizeErrorTolerance);
  };
}

#endif
