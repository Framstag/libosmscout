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
#include <cstring>

namespace osmscout {

  class LineSegment CLASS_FINAL
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

    bool IsValid() const
    {
      return !(xdelta==0 && ydelta==0);
    }

    double CalculateDistanceSquared(const TransPolygon::TransPoint& p) const
    {
      double cx=p.x-ref.x;
      double cy=p.y-ref.y;
      double u=(cx*xdelta+cy*ydelta)*inverseLength;

      u=std::min(1.0,std::max(0.0,u));

      double dx=cx-u*xdelta; // *-1 but we square below
      double dy=cy-u*ydelta; // *-1 but we square below

      return dx*dx+dy*dy;
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

  CoordBuffer::CoordBuffer()
    : bufferSize(131072),
      usedPoints(0),
      buffer(new Vertex2D[bufferSize])
  {
    // no code
  }

  CoordBuffer::~CoordBuffer()
  {
    delete [] buffer;
  }

  void CoordBuffer::Reset()
  {
    usedPoints=0;
  }

  size_t CoordBuffer::PushCoord(double x, double y)
  {
    if (usedPoints>=bufferSize) {
      bufferSize=bufferSize*2;

      auto* newBuffer=new Vertex2D[bufferSize];

      std::memcpy(newBuffer,buffer,sizeof(Vertex2D)*usedPoints);

      log.Warn() << "*** Buffer reallocation: " << bufferSize;

      delete [] buffer;

      buffer=newBuffer;
    }

    buffer[usedPoints].Set(x,y);

    return usedPoints++;
  }

  void CoordBuffer::GenerateParallelWay(size_t orgStart,
                                        size_t orgEnd,
                                        double offset,
                                        size_t& start,
                                        size_t& end)
  {
    assert(orgStart<orgEnd);
    assert(orgEnd<usedPoints);

    double oax,oay;
    double obx,oby;


    Normalize(buffer[orgStart].GetY()-buffer[orgStart+1].GetY(),
              buffer[orgStart+1].GetX()-buffer[orgStart].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    start=PushCoord(buffer[orgStart].GetX()+oax,
                    buffer[orgStart].GetY()+oay);

    for (size_t i=orgStart+1; i<orgEnd; i++) {
      Normalize(buffer[i-1].GetY()-buffer[i].GetY(),
                buffer[i].GetX()-buffer[i-1].GetX(),
                oax, oay);

      oax=offset*oax;
      oay=offset*oay;

      Normalize(buffer[i].GetY()-buffer[i+1].GetY(),
                buffer[i+1].GetX()-buffer[i].GetX(),
                obx, oby);

      obx=offset*obx;
      oby=offset*oby;


      double det1=Det(obx-oax,
                      oby-oay,
                      buffer[i+1].GetX()-buffer[i].GetX(),
                      buffer[i+1].GetY()-buffer[i].GetY());
      double det2=Det(buffer[i].GetX()-buffer[i-1].GetX(),
                      buffer[i].GetY()-buffer[i-1].GetY(),
                      buffer[i+1].GetX()-buffer[i].GetX(),
                      buffer[i+1].GetY()-buffer[i].GetY());

      if (fabs(det2)>0.0001) {
        double addX = det1/det2*(buffer[i].GetX()-buffer[i-1].GetX());
        double addY = det1/det2*(buffer[i].GetY()-buffer[i-1].GetY());
        if (std::abs(addX) < 2*std::abs(offset) && std::abs(addY) < 2*std::abs(offset)) {
          PushCoord(buffer[i].GetX() + oax + addX,
                    buffer[i].GetY() + oay + addY);
        }else{
          // cut the edge of too sharp angles
          PushCoord(buffer[i].GetX() + oax,
                    buffer[i].GetY() + oay);
          PushCoord(buffer[i].GetX() + obx,
                    buffer[i].GetY() + oby);
        }
      }
      else {
        PushCoord(buffer[i].GetX()+oax,
                  buffer[i].GetY()+oay);
      }
    }

    Normalize(buffer[orgEnd-1].GetY()-buffer[orgEnd].GetY(),
              buffer[orgEnd].GetX()-buffer[orgEnd-1].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    end=PushCoord(buffer[orgEnd].GetX()+oax,
                  buffer[orgEnd].GetY()+oay);
  }

  TransPolygon::TransPolygon()
  : pointsSize(0),
    length(0),
    start(0),
    end(0),
    points(nullptr)
  {
    // no code
  }

  TransPolygon::~TransPolygon()
  {
    delete [] points;
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

  void TransPolygon::DropEqualPoints()
  {
    size_t current=0;
    while (current<length) {
      if (!points[current].draw) {
        current++;
        continue;
      }

      size_t next=current+1;
      while (next<length && !points[next].draw) {
        next++;
      }

      if (next>=length) {
        return;
      }

      if (points[current].x==points[next].x &&
          points[current].y==points[next].y) {
        points[next].draw=false;
      }

      current=next;
    }
  }

  void TransPolygon::EnsureSimple(bool isArea)
  {
    // copy points to vector of TransPointRef for easy manipulation
    std::vector<TransPointRef> optimised;

    for (size_t i=0;i<length;i++) {
      if (points[i].draw) {
        TransPointRef ref{points+i};

        optimised.push_back(ref);
      }
    }

    if (optimised.size()<=3) {
      return;
    }
    if (isArea) {
      optimised.push_back(optimised.front());
    }

    bool modified=false;
    bool isSimple=false;
    while (!isSimple) {
      isSimple=true;

      size_t i=0;
      size_t j;
      bool updated=true;
      // following while is just performance optimisation,
      // we don't start searching intersections from start again
      while (updated) {
        if (FindIntersection(optimised,i,j)) {
          isSimple=false;
          modified=true;
          if (isArea && j-i > i+(optimised.size()-j)){
            optimised.erase(optimised.begin()+j+1, optimised.end());
            optimised.erase(optimised.begin(), optimised.begin()+i);
            optimised.push_back(optimised.front());
            i=0;
          }
          else {
            optimised.erase(optimised.begin()+i+1, optimised.begin()+j+1);
          }
        }
        else {
          updated=false;
        }
      }
    }
    if (modified) {
      // setup draw property for points remaining in optimised vector
      for (size_t i=0;i<length;i++) {
        points[i].draw=false;
      }

      for (TransPointRef &ref:optimised) {
        ref.p->draw=true;
      }
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

  void TransPolygon::TransformBoundingBox(const Projection& projection,
                                          TransPolygon::OptimizeMethod optimize,
                                          const GeoBox& boundingBox,
                                          double optimizeErrorTolerance,
                                          TransPolygon::OutputConstraint constraint)
  {
    std::array<GeoCoord,4> coords;

    // left bottom
    coords[0]=GeoCoord(boundingBox.GetMinLat(),
                       boundingBox.GetMinLon());
    // left top
    coords[1]=GeoCoord(boundingBox.GetMaxLat(),
                       boundingBox.GetMinLon());
    // right top
    coords[2]=GeoCoord(boundingBox.GetMaxLat(),
                       boundingBox.GetMaxLon());
    // right bottom
    coords[3]=GeoCoord(boundingBox.GetMinLat(),
                       boundingBox.GetMaxLon());

    TransformArea(projection,
                  optimize,
                  coords,
                  optimizeErrorTolerance,
                  constraint);
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

  void TransBuffer::TransformBoundingBox(const Projection& projection,
                                         TransPolygon::OptimizeMethod optimize,
                                         const GeoBox& boundingBox,
                                         size_t& start,
                                         size_t& end,
                                         double optimizeErrorTolerance)
  {
    transPolygon.TransformBoundingBox(projection, optimize, boundingBox, optimizeErrorTolerance);

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
}
