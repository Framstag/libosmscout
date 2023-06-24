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

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/projection/Projection.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/log/Logger.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  /**
   * Entry in the TransBuffer structure.
   *
   * A transformation object has a coordinate and a flag that signals that it should be either drawn or dropped.
   * Singular coordinates can be dropped due to optimizations with the goal to reduce rendering nodes without
   * sacrificing display quality.
   *
   * \ingroup Geometry
   */
  struct OSMSCOUT_API TransPoint
  {
    bool   draw;
    double x;
    double y;
  };

  /**
   * Temporary stateful buffer for holding results of transformation of polygon from geo coords to display coords.
   *
   * \ingroup Geometry
   */
  class OSMSCOUT_API TransBuffer CLASS_FINAL
  {
  private:
    size_t pointsSize=0;
    size_t length=0;
    size_t start=0;
    size_t end=0;

  public:
    TransPoint* points=nullptr;

  private:
    void Reserve(size_t size);

  public:
    TransBuffer() = default;
    ~TransBuffer();

    /**
     * Return true, if the TransBuffer holds data (length==0)
     * @return
     */
    bool IsEmpty() const
    {
      return length==0;
    }

    /**
     * Return the number of to be drawn points
     * @return
     */
    size_t GetLength() const
    {
      return length;
    }

    /**
     * Return the inex of the first to be drawn point
     * @return
     */
    size_t GetStart() const
    {
      return start;
    }

    /**
     * Return the last to be drawn point
     * @return
     */
    size_t GetEnd() const
    {
      return end;
    }

    /**
     * Reset length, start and end
     */
    void Reset();

    /**
     * After direct writing access to the points array to have to call this method to
     * correct internal start, end and length variables
     */
    void CalcSize();

    /**
     * Transform the given source array of GeoCoords to DisplayPoints in the buffer
     *
     * @tparam C
     *    a container object like list, vector or array
     * @param projection
     *    The projection to use for transformation from geo coordinates to display coordinates
     * @param nodes
     *    The container holding the geo coordinates
     */
    template<typename C>
    void  TransformGeoToPixel(const Projection& projection,
                              const C& nodes)
    {
      Projection::BatchTransformer batchTransformer(projection);

      if (!nodes.empty()) {
        Reserve(nodes.size());

        start=0;
        length=nodes.size();
        end=length-1;

        for (size_t i=start; i<=end; i++) {
          batchTransformer.GeoToPixel(nodes[i],
                                      points[i].x,
                                      points[i].y);
          points[i].draw=true;
        }
      }
    }

    /**
     * Return the bounding box of the to be drawn display coordinates
     *
     * @param xmin
     * @param ymin
     * @param xmax
     * @param ymax
     * @return
     */
    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;
  };

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
  public:
    enum OptimizeMethod
    {
      none   =0,
      fast   =1,
      quality=2
    };

    enum OutputConstraint
    {
      noConstraint=0,
      simple      =1
    };
  };

  /**
   * Optimize a already transformed area
   */
  extern OSMSCOUT_API void OptimizeArea(TransBuffer& buffer,
                                        TransPolygon::OptimizeMethod optimize,
                                        double optimizeErrorTolerance,
                                        TransPolygon::OutputConstraint constraint);

  /**
   * Optimize a already transformed way
   */
  extern OSMSCOUT_API void OptimizeWay(TransBuffer& buffer,
                                       TransPolygon::OptimizeMethod optimize,
                                       double optimizeErrorTolerance,
                                       TransPolygon::OutputConstraint constraint);

  /**
   * Transform form geo to screen coordinates and (optionally) optimize the passed area with the given coordinates
   * @tparam C container type for the geo coordinates of the area
   * @param nodes
   *    Area coordinates
   * @param buffer
   *    TransBuffer instance for memory caching
   * @param projection
   *    Projection to use
   * @param optimize
   *    Mode of optimization
   * @param optimizeErrorTolerance
   * @param constraint
   */
  template<typename C>
  void TransformArea(const C& nodes,
                     TransBuffer& buffer,
                     const Projection& projection,
                     TransPolygon::OptimizeMethod optimize,
                     double optimizeErrorTolerance,
                     TransPolygon::OutputConstraint constraint=TransPolygon::noConstraint)
  {
    buffer.Reset();

    if (nodes.size()<2) {
      return;
    }

    buffer.TransformGeoToPixel(projection,
                               nodes);

    if (optimize!=TransPolygon::none) {
      OptimizeArea(buffer,
                   optimize,
                   optimizeErrorTolerance,
                   constraint);
    }
  }

  /**
   * Transform form geo to screen coordinates and (optionally) optimize the passed way with the given coordinates
   * @tparam C container type for the geo coordinates of the area
   * @param nodes
   *    Area coordinates
   * @param buffer
   *    TransBuffer instance for memory caching
   * @param projection
   *    Projection to use
   * @param optimize
   *    Mode of optimization
   * @param optimizeErrorTolerance
   * @param constraint
   */
  template<typename C>
  void TransformWay(const C& nodes,
                    TransBuffer& buffer,
                    const Projection& projection,
                    TransPolygon::OptimizeMethod optimize,
                    double optimizeErrorTolerance,
                    TransPolygon::OutputConstraint constraint=TransPolygon::noConstraint)
  {
    buffer.Reset();

    if (nodes.empty()) {
      return;
    }

    buffer.TransformGeoToPixel(projection,
                               nodes);

    if (optimize!=TransPolygon::none) {
      OptimizeWay(buffer,
                  optimize,
                  optimizeErrorTolerance,
                  constraint);
    }
  }


  /**
   * Transform form geo to screen coordinates and (optionally) optimize the passed way with the given coordinates
   * @tparam C
   *    Container type for the geo coordinates of the area
   * @param boundingBox
   *    bounding box
   * @param buffer
   *    TransBuffer instance for memory caching
   * @param projection
   *    Projection to use
   * @param optimize
   *    Mode of optimization
   * @param optimizeErrorTolerance
   * @param constraint
   */
  extern OSMSCOUT_API void TransformBoundingBox(const GeoBox& boundingBox,
                                                TransBuffer& buffer,
                                                const Projection& projection,
                                                TransPolygon::OptimizeMethod optimize,
                                                double optimizeErrorTolerance,
                                                TransPolygon::OutputConstraint constraint=TransPolygon::noConstraint);

  // Forward declaration for CoordBuffer
  class CoordBufferRange;

  /**
   * Buffer structure for Vertex2D data. You can add coordinates to the buffer and get the position
   * of the coordinate in the buffer in return.
   *
   * The CoordBuffer automatically resizes by a factor of 2 if its is too small to hold the additional data.
   * The initial size of the buffer should be able to hold "enough" data. If you thus get reallocation
   * log warnings this is not an error, but if it happens too often you are either not reusing
   * CoordBuffer instances as much as possible or are pushing more geometric data than we expect to
   * be sensible for mobile or desktop rendering. Check your allocation strategy for MapPainter instances
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
    size_t   bufferSize=131072;
    size_t   usedPoints=0;

  public:
    static CoordBuffer emptyCoordBuffer;

  public:
    Vertex2D *buffer{new Vertex2D[bufferSize]()};

  public:
    CoordBuffer() = default;
    ~CoordBuffer();

    CoordBuffer(const CoordBuffer& other) = delete;
    CoordBuffer& operator=(const CoordBuffer& other) = delete;

    void Reset();

    /**
     * Push coordinate to the buffer.
     *
     * @param coord
     * @return position (index) of the new coordinate coordinate in the buffer
     * @note x and y have to be valid, NaN is not allowed
     */
    size_t PushCoord(const Vertex2D& coord);

    /**
     * Generate parallel way to way stored in this buffer on range orgStart, orgEnd (inclusive)
     * Result is stored after the last valid point. Generated way offsets are returned
     * in start and end.
     *
     * Way have to have at least two nodes (orgEnd > orgStart)
     *
     * @param org original range of data in the CoordBuffer
     * @param offset offset of parallel way - positive offset is left, negative right
     * @return range of data in the CoordBuffer
     */
    CoordBufferRange GenerateParallelWay(const CoordBufferRange& org,
                                         double offset);
  };

  /**
 * Hold a reference to a range of data within a CoordBuffer.
 *
 * \ingroup Geometry
 */
  class OSMSCOUT_API CoordBufferRange CLASS_FINAL
  {
  private:
    CoordBuffer* coordBuffer=&CoordBuffer::emptyCoordBuffer;
    size_t start=std::numeric_limits<size_t>::max();
    size_t end=std::numeric_limits<size_t>::max();
    mutable double length=-1;

  public:
    CoordBufferRange() = default;
    CoordBufferRange(const CoordBufferRange& other) = default;

    CoordBufferRange(CoordBuffer& coordBuffer, size_t start, size_t end)
      : coordBuffer(&coordBuffer),
        start(start),
        end(end)
    {
    }

    CoordBufferRange& operator=(const CoordBufferRange& other)
    {
      if (this!=&other) {
        this->coordBuffer=other.coordBuffer;
        this->start=other.start;
        this->end=other.end;
      }

      return *this;
    }

    /**
     * Return the first element on the path
     * @return first element
     */
    Vertex2D GetFirst() const
    {
      return coordBuffer->buffer[start];
    }

    /**
     * Return the last element on the path
     * @return last element
     */
    Vertex2D GetLast() const
    {
      return coordBuffer->buffer[end];
    }

    /**
     * Return the index of the first element
     * @return index
     */
    size_t GetStart() const
    {
      return start;
    }

    /**
     * Return the element with the given index
     * @param index where index>=start && index<=end
     * @return the element at the given index
     */
    Vertex2D Get(size_t index) const
    {
      return coordBuffer->buffer[index];
    }

    /**
     * THe index of the last element
     * @return index
     */
    size_t GetEnd() const
    {
      return end;
    }

    /**
     * Returns the number of elements (end-start+1)
     * @return number of elements
     */
    size_t GetSize() const
    {
      return end-start+1;
    }

    /**
     * Returns the on-screen length of the path from the first to the last element
     * @return length of path
     */
    double GetLength() const
    {
      if (length<0.0) {
        Vertex2D lastCoord=GetFirst();

        length=0.0;
        for (size_t j = start+1; j <= end; j++) {
          Vertex2D currentCoord=Get(j);
          length += sqrt(pow(currentCoord.GetX() - lastCoord.GetX(), 2) +
                         pow(currentCoord.GetY() - lastCoord.GetY(), 2));

          lastCoord=currentCoord;
        }
      }

      return length;
    }

    bool IsValid() const {
      return start!=std::numeric_limits<size_t>::max();
    }
  };

  extern OSMSCOUT_API CoordBufferRange CopyPolygonToCoordBuffer(const TransBuffer& transBuffer,
                                                                CoordBuffer& coordBuffer);

  /**
   * Transform the geo coordinates to display coordinates of the given area and copy the resulting coordinates
   * the the given CoordBuffer.
   *
   * @tparam C
   *    Container type for the geo coordinates of the area
   * @param nodes
   *    area coordinates
   * @param transBuffer
   *    TransBuffer instance for memory caching
   * @param coordBuffer
   *    Target CoordBuffer
   * @param projection
   *    Projection to use
   * @param optimize
   * @param optimizeErrorTolerance
   * @return
   *    The resulting coordinate range in the CoordBuffer
   */
  template<typename C>
  CoordBufferRange TransformArea(const C& nodes,
                                 TransBuffer& transBuffer,
                                 CoordBuffer& coordBuffer,
                                 const Projection& projection,
                                 TransPolygon::OptimizeMethod optimize,
                                 double optimizeErrorTolerance)
  {
    TransformArea(nodes,
                  transBuffer,
                  projection,
                  optimize,
                  optimizeErrorTolerance);

    assert(!transBuffer.IsEmpty());

    return CopyPolygonToCoordBuffer(transBuffer,
                                    coordBuffer);
  }

  /**
   * Transform the geo coordinates to display coordinates of the given way and copy the resulting coordinates
   * the the given CoordBuffer.
   *
   * @tparam C
   *    Container type for the geo coordinates of the area
   * @param nodes
   *    way coordinates
   * @param transBuffer
   *    TransBuffer instance for memory caching
   * @param coordBuffer
   *    Target CoordBuffer
   * @param projection
   *    Projection to use
   * @param optimize
   * @param optimizeErrorTolerance
   * @return
   *    The resulting coordinate range in the CoordBuffer
   */
  template<typename C>
  CoordBufferRange TransformWay(const C& nodes,
                                TransBuffer& transBuffer,
                                CoordBuffer& coordBuffer,
                                const Projection& projection,
                                TransPolygon::OptimizeMethod optimize,
                                double optimizeErrorTolerance)
  {
    TransformWay(nodes,
                 transBuffer,
                 projection,
                 optimize,
                 optimizeErrorTolerance);

    assert(!transBuffer.IsEmpty());

    return CopyPolygonToCoordBuffer(transBuffer,
                                    coordBuffer);
  }

  /**
   * Transform the geo coordinates to display coordinates of the given bounding box and copy the resulting coordinates
   * the the given CoordBuffer.
   *
   * @tparam C
   *    Container type for the geo coordinates of the area
   * @param boundingBox
   *    bounding box
   * @param transBuffer
   *    TransBuffer instance for memory caching
   * @param coordBuffer
   *    Target CoordBuffer
   * @param projection
   *    Projection to use
   * @param optimize
   * @param optimizeErrorTolerance
   * @return
   *    The resulting coordinate range in the CoordBuffer
   */
  extern OSMSCOUT_API CoordBufferRange TransformBoundingBox(const GeoBox& boundingBox,
                                                            TransBuffer& buffer,
                                                            CoordBuffer& coordBuffer,
                                                            const Projection& projection,
                                                            TransPolygon::OptimizeMethod optimize,
                                                            double optimizeErrorTolerance);
}

#endif
