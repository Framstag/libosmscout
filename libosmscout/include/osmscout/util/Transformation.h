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
   * Class to allows transformation of geometric primitives form geo coordinate to display
   * coordinates using the passed Projection instance.
   *
   * A number of optimizations on the resulting display coordinates objects can be triggered to
   * reduce the number of to-be-rendered pixels.
   *
   * In one pass only one geometric primitive can be transformed, however to reduce memory allocation
   * and reallocation TransPolygon can be reused.
   *
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
    template <typename C>
    void  TransformGeoToPixel(const Projection& projection,
                              const C& nodes)
    {
      Projection::BatchTransformer batchTransformer(projection);

      if (!nodes.empty()) {
        start=0;
        length=nodes.size();
        end=length-1;

        for (size_t i=start; i<=end; i++) {
          batchTransformer.GeoToPixel(nodes[i].GetLon(),
                                      nodes[i].GetLat(),
                                      points[i].x,
                                      points[i].y);
          points[i].draw=true;
        }
      }
      else {
        start=0;
        end=0;
        length=0;
      }
    }

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

    template<typename C>
    void TransformArea(const Projection& projection,
                       OptimizeMethod optimize,
                       const C& nodes,
                       double optimizeErrorTolerance,
                       OutputConstraint constraint=noConstraint)
    {
      if (nodes.size()<2) {
        length=0;

        return;
      }

      if (pointsSize<nodes.size()) {
        delete[] points;

        points=new TransPoint[nodes.size()];
        pointsSize=nodes.size();
      }

      TransformGeoToPixel(projection,
                          nodes);

      if (optimize!=none) {
        if (optimize==fast) {
          DropSimilarPoints(optimizeErrorTolerance);
          DropRedundantPointsFast(optimizeErrorTolerance);
        }
        else {
          DropRedundantPointsDouglasPeucker(optimizeErrorTolerance,
                                            true);
        }

        DropEqualPoints();

        if (constraint==simple) {
          EnsureSimple(true);
        }

        length=0;
        start=nodes.size();
        end=0;

        // Calculate start, end and length
        for (size_t i=0; i<nodes.size(); i++) {
          if (points[i].draw) {
            length++;

            if (i<start) {
              start=i;
            }

            end=i;
          }
        }
      }
    }

    template<typename C>
    void TransformWay(const Projection& projection,
                      OptimizeMethod optimize,
                      const C& nodes,
                      double optimizeErrorTolerance,
                      OutputConstraint constraint=noConstraint)
    {
      if (nodes.empty()) {
        length=0;

        return;
      }

      if (pointsSize<nodes.size()) {
        delete [] points;

        points=new TransPoint[nodes.size()];
        pointsSize=nodes.size();
      }

      TransformGeoToPixel(projection,
                          nodes);

      if (optimize!=none) {

        DropSimilarPoints(optimizeErrorTolerance);

        if (optimize==fast) {
          DropRedundantPointsFast(optimizeErrorTolerance);
        }
        else {
          DropRedundantPointsDouglasPeucker(optimizeErrorTolerance,false);
        }

        DropEqualPoints();

        if (constraint==simple) {
          EnsureSimple(false);
        }

        length=0;
        start=nodes.size();
        end=0;

        // Calculate start & end
        for (size_t i=0; i<nodes.size(); i++) {
          if (points[i].draw) {
            length++;

            if (i<start) {
              start=i;
            }
            end=i;
          }
        }
      }
    }

    void TransformBoundingBox(const Projection& projection,
                              OptimizeMethod optimize,
                              const GeoBox& boundingBox,
                              double optimizeErrorTolerance,
                              OutputConstraint constraint=noConstraint);

    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;
  };

  /**
   * Buffer structure for Vertex2D data. You can add coordinates to the buffer and get the position
   * of the coordinate in the buffer in return.
   *
   * The CoordBuffer automatically resizes by a factor of 2 if its is too small to hold the additional data.
   * The initial size of the buffer should be able to hold "enough" data. If you thus get reallocation
   * log warnings this is not an error, but if it happens too often you are either not reusing
   * CoordBuffer instances as much as possible or are pushing more geometric data than we expect to
   * be sensible for mobile or desktop rendering. Check your allocation strategy for MapPainte rinstances
   * or style sheet in this case,
   *
   * CoordBuffer also allows also higher level operations on the buffer to generate copies
   * of stored objects.
   *
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
     * Way have to have at least two nodes (orgEnd > orgStart)
     *
     * @param orgStart original way start
     * @param orgEnd original way end (inclusive)
     * @param offset offset of parallel way - positive offset is left, negative right
     * @param start start of result
     * @param end end of result (inclusive)
     */
    void GenerateParallelWay(size_t orgStart,
                             size_t orgEnd,
                             double offset,
                             size_t& start,
                             size_t& end);
  };


  /**
   * Allows to transform areas and ways using a embedded TransPolygon (for caching od data structures
   * to avoid allocations) and copies the result to the passed CoordBuffer.
   *
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
    bool TransformBoundingBox(const Projection& projection,
                              TransPolygon::OptimizeMethod optimize,
                              const GeoBox& boundingBox,
                              size_t& start, size_t &end,
                              double optimizeErrorTolerance);
  };
}

#endif
