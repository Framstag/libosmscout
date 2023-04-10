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

#include <cstring>

#include <limits>

namespace osmscout {

  TransBuffer::~TransBuffer()
  {
    delete [] points;
  }

  void TransBuffer::Reset()
  {
    length=0;
    start=0;
    end=0;
  }

  void TransBuffer::CalcSize()
  {
    start=length; // We now that length is either 0 or set by TransformGeoToPixel()
    size_t dataSize=length;
    end=0;

    // Calculate start, end and length
    length=0;
    for (size_t i=0; i<dataSize; i++) {
      if (points[i].draw) {
        length++;

        if (i<start) {
          start=i;
        }

        end=i;
      }
    }
  }

  void TransBuffer::Reserve(size_t size)
  {
    if (pointsSize<size) {
      delete[] points;

      points=new TransPoint[size];
      pointsSize=size;
    }
  }

  bool TransBuffer::GetBoundingBox(double& xmin, double& ymin,
                                   double& xmax, double& ymax) const
  {
    if (IsEmpty()) {
      return false;
    }

    size_t pos=GetStart();

    while (!points[pos].draw) {
      pos++;
    }

    xmin=points[pos].x;
    xmax=xmin;
    ymin=points[pos].y;
    ymax=ymin;

    while (pos<=GetEnd()) {
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

  class LineSegment CLASS_FINAL
  {
  private:
    TransPoint ref;
    double     xdelta;
    double     ydelta;
    double     inverseLength;

  public:

    LineSegment(const TransPoint& a,
                const TransPoint& b)
    :ref(a)
    {
      xdelta=b.x-a.x;
      ydelta=b.y-a.y;
      inverseLength=1/(xdelta*xdelta+ydelta*ydelta);
    }

    bool IsValid() const
    {
      return xdelta!=0 || ydelta!=0;
    }

    double CalculateDistanceSquared(const TransPoint& p) const
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
  static double CalculateDistancePointToLineSegment(const TransPoint& p,
                                                    const TransPoint& a,
                                                    const TransPoint& b)
  {
    double xdelta=b.x-a.x;
    double ydelta=b.y-a.y;

    if (xdelta==0 && ydelta==0) {
      return std::numeric_limits<double>::infinity();
    }

    double u=((p.x-a.x)*xdelta+(p.y-a.y)*ydelta)/(xdelta*xdelta+ydelta*ydelta);

    double cx;
    double cy;

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

  static double CalculateDistancePointToPoint(const TransPoint& a,
                                              const TransPoint& b)
  {
    double xdelta=b.x-a.x;
    double ydelta=b.y-a.y;

    return sqrt(xdelta*xdelta + ydelta*ydelta);
  }

  static void SimplifyPolyLineDouglasPeucker(TransPoint* points,
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

  // Global empty default coord buffer for default constructor of CoordBufferRange
  CoordBuffer CoordBuffer::emptyCoordBuffer;

  void CoordBuffer::Reset()
  {
    usedPoints=0;
  }

  size_t CoordBuffer::PushCoord(double x, double y)
  {
    assert(!std::isnan(x));
    assert(!std::isnan(y));

    if (usedPoints>=bufferSize) {
      bufferSize=bufferSize*2;

      Vertex2D *newBuffer=new Vertex2D[bufferSize];

      std::memcpy(newBuffer,buffer,sizeof(Vertex2D)*usedPoints);

      log.Warn() << "*** Buffer reallocation: " << bufferSize;

      delete [] buffer;

      buffer=newBuffer;
    }

    buffer[usedPoints]=Vertex2D(x,y);

    return usedPoints++;
  }

  size_t CoordBuffer::PushCoord(const Vertex2D& coord)
  {
    assert(!std::isnan(coord.GetX()));
    assert(!std::isnan(coord.GetY()));

    if (usedPoints>=bufferSize) {
      bufferSize=bufferSize*2;

      Vertex2D *newBuffer=new Vertex2D[bufferSize];

      std::memcpy(newBuffer,buffer,sizeof(Vertex2D)*usedPoints);

      log.Warn() << "*** Buffer reallocation: " << bufferSize;

      delete [] buffer;

      buffer=newBuffer;
    }

    buffer[usedPoints]=coord;

    return usedPoints++;
  }

  CoordBufferRange CoordBuffer::GenerateParallelWay(const CoordBufferRange& org,
                                                    double offset)
  {
    assert(org.GetStart()<org.GetEnd());
    assert(org.GetEnd()<usedPoints);

    size_t start;
    size_t end;
    double oax=0;
    double oay=0;
    double obx=0;
    double oby=0;

    Normalize(buffer[org.GetStart()].GetY()-buffer[org.GetStart()+1].GetY(),
              buffer[org.GetStart()+1].GetX()-buffer[org.GetStart()].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    start=PushCoord(Vertex2D(buffer[org.GetStart()].GetX()+oax,
                             buffer[org.GetStart()].GetY()+oay));

    for (size_t i=org.GetStart()+1; i<org.GetEnd(); i++) {
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
          PushCoord(Vertex2D(buffer[i].GetX() + oax + addX,
                             buffer[i].GetY() + oay + addY));
        }else{
          // cut the edge of too sharp angles
          PushCoord(Vertex2D(buffer[i].GetX() + oax,
                             buffer[i].GetY() + oay));
          PushCoord(Vertex2D(buffer[i].GetX() + obx,
                             buffer[i].GetY() + oby));
        }
      }
      else {
        PushCoord(Vertex2D(buffer[i].GetX()+oax,
                           buffer[i].GetY()+oay));
      }
    }

    Normalize(buffer[org.GetEnd()-1].GetY()-buffer[org.GetEnd()].GetY(),
              buffer[org.GetEnd()].GetX()-buffer[org.GetEnd()-1].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    end=PushCoord(Vertex2D(buffer[org.GetEnd()].GetX()+oax,
                           buffer[org.GetEnd()].GetY()+oay));

    return CoordBufferRange(*this,start,end);
  }

  CoordBuffer::~CoordBuffer()
  {
    delete [] buffer;
  }

  static void DropSimilarPoints(TransBuffer &buffer,double optimizeErrorTolerance)
  {
    for (size_t i=0; i<buffer.GetLength(); i++) {
      if (buffer.points[i].draw) {
        size_t j=i+1;
        while (j<buffer.GetLength()-1) {
          if (buffer.points[j].draw)
          {
            if (std::fabs(buffer.points[j].x-buffer.points[i].x)<=optimizeErrorTolerance &&
                std::fabs(buffer.points[j].y-buffer.points[i].y)<=optimizeErrorTolerance) {
              buffer.points[j].draw=false;
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

  static void DropRedundantPointsFast(TransBuffer& buffer,double optimizeErrorTolerance)
  {
    // Drop every point that is (more or less) on direct line between two points A and B
    size_t prev=0;
    while (prev<buffer.GetLength()) {

      while (prev<buffer.GetLength() && !buffer.points[prev].draw) {
        prev++;
      }

      if (prev>=buffer.GetLength()) {
        break;
      }

      size_t cur=prev+1;

      while (cur<buffer.GetLength() && !buffer.points[cur].draw) {
        cur++;
      }

      if (cur>=buffer.GetLength()) {
        break;
      }

      size_t next=cur+1;

      while (next<buffer.GetLength() && !buffer.points[next].draw) {
        next++;
      }

      if (next>=buffer.GetLength()) {
        break;
      }

      double distance=CalculateDistancePointToLineSegment(buffer.points[cur],
                                                          buffer.points[prev],
                                                          buffer.points[next]);

      if (distance<=optimizeErrorTolerance) {
        buffer.points[cur].draw=false;

        prev=next;
      }
      else {
        prev++;
      }
    }
  }

  static void DropRedundantPointsDouglasPeuckerArea(TransBuffer& buffer, double optimizeErrorTolerance)
  {
    // An implementation of Douglas-Peuker algorithm http://softsurfer.com/Archive/algorithm_0205/algorithm_0205.htm

    double optimizeErrorToleranceSquared=optimizeErrorTolerance*optimizeErrorTolerance;
    size_t begin=0;

    while (begin<buffer.GetLength() &&
           !buffer.points[begin].draw) {
      begin++;
    }

    if (begin>=buffer.GetLength()) {
      return; //we found no single point that is drawn.
    }

    //if this polygon is an area, we start by finding the largest distance from begin point to all other points
    double maxDist=0.0;
    size_t maxDistIndex=begin;

    for(size_t i=begin; i<buffer.GetLength(); ++i) {
      if (buffer.points[i].draw) {
        double dist=CalculateDistancePointToPoint(buffer.points[begin], buffer.points[i]);

        if (dist>maxDist) {
          maxDist=dist;
          maxDistIndex=i;
        }
      }
    }

    if (maxDistIndex==begin) {
      return; //we only found 1 point to draw
    }

    SimplifyPolyLineDouglasPeucker(buffer.points,
                                   begin,
                                   maxDistIndex,
                                   maxDistIndex,
                                   optimizeErrorToleranceSquared);
    SimplifyPolyLineDouglasPeucker(buffer.points,
                                   maxDistIndex,
                                   buffer.GetLength(),
                                   begin,
                                   optimizeErrorToleranceSquared);
  }

  static void DropRedundantPointsDouglasPeuckerWay(TransBuffer& buffer, double optimizeErrorTolerance)
  {
    // An implementation of Douglas-Peuker algorithm http://softsurfer.com/Archive/algorithm_0205/algorithm_0205.htm

    double optimizeErrorToleranceSquared=optimizeErrorTolerance*optimizeErrorTolerance;
    size_t begin=0;

    while (begin<buffer.GetLength() &&
           !buffer.points[begin].draw) {
      begin++;
    }

    if (begin>=buffer.GetLength()) {
      return; //we found no single point that is drawn.
    }

    //find last drawable point;
    size_t end=buffer.GetLength()-1;
    while (end>begin &&
           !buffer.points[end].draw) {
      end--;
    }

    if (end<=begin) {
      return; //we only found 1 drawable point;
    }

    SimplifyPolyLineDouglasPeucker(buffer.points,
                                   begin,
                                   end,
                                   end,
                                   optimizeErrorToleranceSquared);
  }

  static void DropEqualPoints(TransBuffer& buffer)
  {
    size_t current=0;
    while (current<buffer.GetLength()) {
      if (!buffer.points[current].draw) {
        current++;
        continue;
      }

      size_t next=current+1;
      while (next<buffer.GetLength() && !buffer.points[next].draw) {
        next++;
      }

      if (next>=buffer.GetLength()) {
        return;
      }

      if (buffer.points[current].x==buffer.points[next].x &&
        buffer.points[current].y==buffer.points[next].y) {
        buffer.points[next].draw=false;
      }

      current=next;
    }
  }

  static void EnsureSimple(TransBuffer& buffer, bool isArea)
  {
    struct TransPointRef
    {
      TransPoint *p;

      double GetLat() const
      {
        return p->x;
      }

      double GetLon() const
      {
        return p->y;
      }

      bool IsEqual(const TransPointRef &other) const
      {
        return p==other.p;
      }
    };

    // copy points to vector of TransPointRef for easy manipulation
    std::vector<TransPointRef> optimised;

    for (size_t i=0;i<buffer.GetLength();i++) {
      if (buffer.points[i].draw) {
        TransPointRef ref{buffer.points+i};

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
      for (size_t i=0;i<buffer.GetLength();i++) {
        buffer.points[i].draw=false;
      }

      for (TransPointRef &ref:optimised) {
        ref.p->draw=true;
      }
    }
  }

  void OptimizeArea(TransBuffer& buffer,
                    TransPolygon::OptimizeMethod optimize,
                    double optimizeErrorTolerance,
                    TransPolygon::OutputConstraint constraint)
  {
    if (optimize==TransPolygon::fast) {
      DropSimilarPoints(buffer,optimizeErrorTolerance);
      DropRedundantPointsFast(buffer,optimizeErrorTolerance);
    }
    else {
      DropRedundantPointsDouglasPeuckerArea(buffer,optimizeErrorTolerance);
    }

    DropEqualPoints(buffer);

    if (constraint==TransPolygon::simple) {
      EnsureSimple(buffer,true);
    }

    buffer.CalcSize();
  }

  void OptimizeWay(TransBuffer& buffer,
                   TransPolygon::OptimizeMethod optimize,
                   double optimizeErrorTolerance,
                   TransPolygon::OutputConstraint constraint)
  {
    DropSimilarPoints(buffer,optimizeErrorTolerance);

    if (optimize==TransPolygon::fast) {
      DropRedundantPointsFast(buffer,optimizeErrorTolerance);
    }
    else {
      DropRedundantPointsDouglasPeuckerWay(buffer,optimizeErrorTolerance);
    }

    DropEqualPoints(buffer);

    if (constraint==TransPolygon::simple) {
      EnsureSimple(buffer,false);
    }

    buffer.CalcSize();
  }

  void TransformBoundingBox(const GeoBox& boundingBox,
                            TransBuffer& buffer,
                            const Projection& projection,
                            TransPolygon::OptimizeMethod optimize,
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

    TransformArea(coords,
                  buffer,
                  projection,
                  optimize,
                  optimizeErrorTolerance,
                  constraint);
  }

  CoordBufferRange CopyPolygonToCoordBuffer(const TransBuffer& transBuffer, CoordBuffer& coordBuffer)
  {
    assert(!transBuffer.IsEmpty());

    size_t start=0;
    size_t end=0;
    bool   isStart=true;

    for (size_t i=transBuffer.GetStart(); i<=transBuffer.GetEnd(); i++) {
      if (transBuffer.points[i].draw) {
        end=coordBuffer.PushCoord(Vertex2D(transBuffer.points[i].x,
                                           transBuffer.points[i].y));

        if (isStart) {
          start=end;
          isStart=false;
        }
      }
    }

    return CoordBufferRange(coordBuffer,start,end);
  }

  CoordBufferRange TransformBoundingBox(const GeoBox& boundingBox,
                                        TransBuffer& transBuffer,
                                        CoordBuffer& coordBuffer,
                                        const Projection& projection,
                                        TransPolygon::OptimizeMethod optimize,
                                        double optimizeErrorTolerance)
  {
    TransformBoundingBox(boundingBox,
                         transBuffer,
                         projection,
                         optimize,
                         optimizeErrorTolerance);

    assert(!transBuffer.IsEmpty());

    return CopyPolygonToCoordBuffer(transBuffer,
                                    coordBuffer);
  }
}
