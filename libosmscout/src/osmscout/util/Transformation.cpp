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

#include <osmscout/util/Transformation.h>

#include <limits>

namespace osmscout {

  class LineSegment
  {
  private:
    TransPolygon::TransPoint ref;
    double                   xdelta;
    double                   ydelta;
    double                   inverseLength;

  public:

    LineSegment(const TransPolygon::TransPoint& a,
                const TransPolygon::TransPoint& b)
    :ref(a)
    {
      xdelta=b.x-a.x;
      ydelta=b.y-a.y;
      inverseLength=1/(xdelta*xdelta+ydelta*ydelta);
    }

    bool IsValid()
    {
      return !(xdelta==0 && ydelta==0);
    }

    double CalculateDistanceSquared(const TransPolygon::TransPoint& p)
    {
      double cx=p.x-ref.x;
      double cy=p.y-ref.y;
      double u=(cx*xdelta+cy*ydelta)*inverseLength;

      u=std::min(1.0,std::max(0.0,u));

      double dx=cx-u*xdelta; // *-1 but we square below
      double dy=cy-u*ydelta; // *-1 but we square below

      return dx*dx+dy*dy;
    }

    double CalculateDistance(const TransPolygon::TransPoint& p)
    {
      return sqrt(CalculateDistanceSquared(p));
    }
  };

  /**
   * Calculates the distance between a point p and a line defined by the points a and b.
   * @param p
   *    The point in distance to a line
   * @param a
   *    One point defining the line
   * @param b
   *    Another point defining the line
   * @return
   *    The distance
   */
  static double CalculateDistancePointToLineSegment(const TransPolygon::TransPoint& p,
                                                    const TransPolygon::TransPoint& a,
                                                    const TransPolygon::TransPoint& b)
  {
    double xdelta=b.x-a.x;
    double ydelta=b.y-a.y;

    if (xdelta==0 && ydelta==0) {
      return std::numeric_limits<double>::infinity();
    }

    double u=((p.x-a.x)*xdelta+(p.y-a.y)*ydelta)/(xdelta*xdelta+ydelta*ydelta);

    double cx,cy;

    if (u<0) {
      cx=a.x;
      cy=a.y;
    }
    else if (u>1) {
      cx=b.x;
      cy=b.y;
    }
    else {
      cx=a.x+u*xdelta;
      cy=a.y+u*ydelta;
    }

    double dx=cx-p.x;
    double dy=cy-p.y;

    return sqrt(dx*dx+dy*dy);
  }

  static double CalculateDistancePointToPoint(const TransPolygon::TransPoint& a,
                                              const TransPolygon::TransPoint& b)
  {
    double xdelta=b.x-a.x;
    double ydelta=b.y-a.y;

    return sqrt(xdelta*xdelta + ydelta*ydelta);
  }

  static void SimplifyPolyLineDouglasPeucker(TransPolygon::TransPoint* points,
                                             size_t beginIndex,
                                             size_t endIndex,
                                             size_t endValueIndex,
                                             double optimizeErrorToleranceSquared)
  {
    LineSegment lineSegment(points[beginIndex], points[endValueIndex]);

    double maxDistanceSquared=0;
    size_t maxDistanceIndex=beginIndex;

    for(size_t i=beginIndex+1; i<endIndex; ++i){
      if (points[i].draw) {
        double distanceSquared=lineSegment.CalculateDistanceSquared(points[i]);

        if (distanceSquared>maxDistanceSquared) {
          maxDistanceSquared=distanceSquared;
          maxDistanceIndex=i;
        }
      }
    }

    if (maxDistanceSquared<=optimizeErrorToleranceSquared) {

      //we don't need to draw any extra points
      for(size_t i=beginIndex+1; i<endIndex; ++i){
        points[i].draw=false;
      }

      return;
    }

    //we need to split this line in two pieces
    SimplifyPolyLineDouglasPeucker(points,
                                   beginIndex,
                                   maxDistanceIndex,
                                   maxDistanceIndex,
                                   optimizeErrorToleranceSquared);

    SimplifyPolyLineDouglasPeucker(points,
                                   maxDistanceIndex,
                                   endIndex,
                                   endValueIndex,
                                   optimizeErrorToleranceSquared);
  }

  TransPolygon::TransPolygon()
  : pointsSize(0),
    length(0),
    start(0),
    end(0),
    points(NULL)
  {
    // no code
  }

  TransPolygon::~TransPolygon()
  {
    delete [] points;
  }

  void TransPolygon::TransformGeoToPixel(const Projection& projection,
                                         const PointSequence& nodes)
  {
    Projection::BatchTransformer batchTransformer(projection);

    if (!nodes.empty()) {
      start=0;
      length=nodes.size();
      end=length-1;
      
      size_t i=0;
      for (PointSequenceIterator it = nodes.begin(); it!=nodes.end(); ++it) {
        Point point = *it;          
        batchTransformer.GeoToPixel(point.GetLon(),
                                    point.GetLat(),
                                    points[i].x,
                                    points[i].y);
        points[i].draw=true;
        i++;
      }
    }
    else {
      start=0;
      end=0;
      length=0;
    }
  }

  void TransPolygon::DropSimilarPoints(double optimizeErrorTolerance)
  {
    for (size_t i=0; i<length; i++) {
      if (points[i].draw) {
        size_t j=i+1;
        while (j<length-1) {
          if (points[j].draw)
          {
            if (std::fabs(points[j].x-points[i].x)<=optimizeErrorTolerance &&
                std::fabs(points[j].y-points[i].y)<=optimizeErrorTolerance) {
              points[j].draw=false;
            }
            else {
              break;
            }
          }

          j++;
        }
      }
    }
  }

  void TransPolygon::DropRedundantPointsFast(double optimizeErrorTolerance)
  {
    // Drop every point that is (more or less) on direct line between two points A and B
    size_t prev=0;
    while (prev<length) {

      while (prev<length && !points[prev].draw) {
        prev++;
      }

      if (prev>=length) {
        break;
      }

      size_t cur=prev+1;

      while (cur<length && !points[cur].draw) {
        cur++;
      }

      if (cur>=length) {
        break;
      }

      size_t next=cur+1;

      while (next<length && !points[next].draw) {
        next++;
      }

      if (next>=length) {
        break;
      }

      double distance=CalculateDistancePointToLineSegment(points[cur],
                                                          points[prev],
                                                          points[next]);

      if (distance<=optimizeErrorTolerance) {
        points[cur].draw=false;

        prev=next;
      }
      else {
        prev++;
      }
    }
  }

  void TransPolygon::DropRedundantPointsDouglasPeucker(double optimizeErrorTolerance,
                                                       bool isArea)
  {
    // An implementation of Douglas-Peuker algorithm http://softsurfer.com/Archive/algorithm_0205/algorithm_0205.htm

    double optimizeErrorToleranceSquared=optimizeErrorTolerance*optimizeErrorTolerance;
    size_t begin=0;

    while (begin<length &&
           !points[begin].draw) {
      begin++;
    }

    if (begin>=length) {
      return; //we found no single point that is drawn.
    }

    //if this polyon is an area, we start by finding the largest distance from begin point to all other points
    if (isArea) {

      double maxDist=0.0;
      size_t maxDistIndex=begin;

      for(size_t i=begin; i<length; ++i) {
        if (points[i].draw) {
          double dist=CalculateDistancePointToPoint(points[begin], points[i]);

          if (dist>maxDist) {
            maxDist=dist;
            maxDistIndex=i;
          }
        }
      }

      if (maxDistIndex==begin) {
        return; //we only found 1 point to draw
      }

      SimplifyPolyLineDouglasPeucker(points,
                                     begin,
                                     maxDistIndex,
                                     maxDistIndex,
                                     optimizeErrorToleranceSquared);
      SimplifyPolyLineDouglasPeucker(points,
                                     maxDistIndex,
                                     length,
                                     begin,
                                     optimizeErrorToleranceSquared);
    }
    else {
      //not an area but polyline
      //find last drawable point;
      size_t end=length-1;
      while (end>begin &&
          !points[end].draw) {
        end--;
      }

      if (end<=begin) {
        return; //we only found 1 drawable point;
      }

      SimplifyPolyLineDouglasPeucker(points,
                                     begin,
                                     end,
                                     end,
                                     optimizeErrorToleranceSquared);
    }
  }

  void TransPolygon::TransformArea(const Projection& projection,
                                   OptimizeMethod optimize,
                                   const std::vector<Point>& nodes,
                                   double optimizeErrorTolerance)
  {
      return TransformArea(projection, optimize, TempVectorPointSequence(&nodes), optimizeErrorTolerance);
  }
  
  void TransPolygon::TransformArea(const Projection& projection,
                                   OptimizeMethod optimize,
                                   const PointSequence& nodes,
                                   double optimizeErrorTolerance)
  {
    if (nodes.size()<2) {
      length=0;

      return;
    }

    if (pointsSize<nodes.size()) {
      delete [] points;

      points=new TransPoint[nodes.size()];
      pointsSize=nodes.size();
    }

    if (optimize!=none) {
      TransformGeoToPixel(projection,
                          nodes);


      if (optimize==fast) {
        DropSimilarPoints(optimizeErrorTolerance);
        DropRedundantPointsFast(optimizeErrorTolerance);
      }
      else {
        DropRedundantPointsDouglasPeucker(optimizeErrorTolerance,true);
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
    else {
      TransformGeoToPixel(projection,
                          nodes);
    }
  }

  void TransPolygon::TransformWay(const Projection& projection,
                                  OptimizeMethod optimize,
                                  const std::vector<Point>& nodes,
                                  double optimizeErrorTolerance)
  {
      return TransformWay(projection, optimize, TempVectorPointSequence(&nodes), optimizeErrorTolerance);
  }
  
  void TransPolygon::TransformWay(const Projection& projection,
                                  OptimizeMethod optimize,
                                  const PointSequence& nodes,
                                  double optimizeErrorTolerance)
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

    if (optimize!=none) {
      TransformGeoToPixel(projection,
                          nodes);

      DropSimilarPoints(optimizeErrorTolerance);

      if (optimize==fast) {
        DropRedundantPointsFast(optimizeErrorTolerance);
      }
      else {
        DropRedundantPointsDouglasPeucker(optimizeErrorTolerance,false);
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
    else {
      TransformGeoToPixel(projection,
                          nodes);
    }
  }

  bool TransPolygon::GetBoundingBox(double& xmin, double& ymin,
                                    double& xmax, double& ymax) const
  {
    if (IsEmpty()) {
      return false;
    }

    size_t pos=start;

    while (!points[pos].draw) {
      pos++;
    }

    xmin=points[pos].x;
    xmax=xmin;
    ymin=points[pos].y;
    ymax=ymin;

    while (pos<=end) {
      if (points[pos].draw) {
        xmin=std::min(xmin,points[pos].x);
        xmax=std::max(xmax,points[pos].x);
        ymin=std::min(ymin,points[pos].y);
        ymax=std::max(ymax,points[pos].y);
      }

      pos++;
    }

    return true;
  }

  bool TransPolygon::GetCenterPixel(double& cx,
                                    double& cy) const
  {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    if (!GetBoundingBox(xmin,ymin,xmax,ymax)) {
      return false;
    }

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;

    return true;
  }

  CoordBuffer::~CoordBuffer()
  {
    // no code
  }

  TransBuffer::TransBuffer(CoordBuffer* buffer)
  : buffer(buffer)
  {
    // no code
  }

  TransBuffer::~TransBuffer()
  {
    delete buffer;
  }

  void TransBuffer::Reset()
  {
    buffer->Reset();
  }

  void TransBuffer::TransformArea(const Projection& projection,
                                  TransPolygon::OptimizeMethod optimize,
                                  const std::vector<Point>& nodes,
                                  size_t& start, size_t &end,
                                  double optimizeErrorTolerance)
  {
      return TransformArea(projection, optimize, TempVectorPointSequence(&nodes), 
              start, end, optimizeErrorTolerance);
  }
  
  void TransBuffer::TransformArea(const Projection& projection,
                                  TransPolygon::OptimizeMethod optimize,
                                  const PointSequence& nodes,
                                  size_t& start, size_t &end,
                                  double optimizeErrorTolerance)
  {
    transPolygon.TransformArea(projection,
                               optimize,
                               nodes,
                               optimizeErrorTolerance);

    assert(!transPolygon.IsEmpty());

    bool isStart=true;
    for (size_t i=transPolygon.GetStart(); i<=transPolygon.GetEnd(); i++) {
      if (transPolygon.points[i].draw) {
        end=buffer->PushCoord(transPolygon.points[i].x,
                              transPolygon.points[i].y);

        if (isStart) {
          start=end;
          isStart=false;
        }
      }
    }
  }

  bool TransBuffer::TransformWay(const Projection& projection,
                                 TransPolygon::OptimizeMethod optimize,
                                 const std::vector<Point>& nodes,
                                 size_t& start, size_t &end,
                                 double optimizeErrorTolerance)
  {
    return TransformWay(projection, optimize, TempVectorPointSequence(&nodes), 
            start, end, optimizeErrorTolerance);
  }

  bool TransBuffer::TransformWay(const Projection& projection,
                                 TransPolygon::OptimizeMethod optimize,
                                 const PointSequence& nodes,
                                 size_t& start, size_t &end,
                                 double optimizeErrorTolerance)
  {
    transPolygon.TransformWay(projection, optimize, nodes, optimizeErrorTolerance);

    if (transPolygon.IsEmpty()) {
      return false;
    }

    bool isStart=true;
    for (size_t i=transPolygon.GetStart(); i<=transPolygon.GetEnd(); i++) {
      if (transPolygon.points[i].draw) {
        end=buffer->PushCoord(transPolygon.points[i].x,
                              transPolygon.points[i].y);

        if (isStart) {
          start=end;
          isStart=false;
        }
      }
    }

    return true;
  }
}
