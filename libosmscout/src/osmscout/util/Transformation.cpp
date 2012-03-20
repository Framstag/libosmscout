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

#include <cassert>
#include <cstring>
#include <limits>

#include <osmscout/private/Math.h>

#include <iostream>

namespace osmscout {
  static double relevantPosDeriviation=1.0;   // Pixel

  static double distancePointToLineSegment(const TransPoint& p, const TransPoint& a, const TransPoint& b)
  {
    double xdelta=b.x-a.x;
    double ydelta=b.y-a.y;

    if (xdelta==0 && ydelta==0) {
      return std::numeric_limits<double>::infinity();
    }

    double u=((p.x-a.x)*xdelta+(p.y-a.y)*ydelta)/(pow(xdelta,2)+pow(ydelta,2));

    double cx,cy;

    if (u<0) {
      u=0;
      cx=a.x;
      cy=a.y;
    }
    else if (u>1) {
      u=1;
      cx=b.x;
      cy=b.y;
    }
    else {
      cx=a.x+u*xdelta;
      cy=a.y+u*ydelta;
    }

    return sqrt(pow(cx-p.x,2)+pow(cy-p.y,2));
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

  void TransPolygon::InitializeDraw()
  {
    for (size_t i=0; i<length; i++) {
      points[i].draw=true;
    }
  }

  void TransPolygon::TransformGeoToPixel(const Projection& projection,
                                         const std::vector<Point>& nodes)
  {
    for (size_t i=0; i<length; i++) {
      if (points[i].draw) {
        projection.GeoToPixel(nodes[i].GetLon(),
                              nodes[i].GetLat(),
                              points[i].x,
                              points[i].y);
      }
    }
  }

  void TransPolygon::DropSimilarPoints()
  {
    for (size_t i=0; i<length; i++) {
      if (points[i].draw) {
        size_t j=i+1;
        while (j<length-1) {
          if (points[j].draw)
          {
            if (std::fabs(points[j].x-points[i].x)<=relevantPosDeriviation &&
                std::fabs(points[j].y-points[i].y)<=relevantPosDeriviation) {
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

  void TransPolygon::DropRedundantPoints()
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

      double distance=distancePointToLineSegment(points[cur],
                                                 points[prev],
                                                 points[next]);

      if (distance<=relevantPosDeriviation) {
        points[cur].draw=false;

        prev=next;
      }
      else {
        prev++;
      }
    }
  }

  void TransPolygon::TransformArea(const Projection& projection,
                                   bool optimize,
                                   const std::vector<Point>& nodes)
  {
    if (nodes.size()<2)
    {
      length=0;
      return;
    }

    length=nodes.size();

    if (pointsSize<length)
    {
      delete [] points;
      points=new TransPoint[length];
      pointsSize=nodes.size();
    }

    if (optimize) {
      InitializeDraw();

      TransformGeoToPixel(projection,
                          nodes);

      DropSimilarPoints();

      DropRedundantPoints();

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
      start=0;
      end=nodes.size()-1;
      length=nodes.size();

      for (size_t i=0; i<nodes.size(); i++) {
        points[i].draw=true;
        projection.GeoToPixel(nodes[i].GetLon(),
                              nodes[i].GetLat(),
                              points[i].x,
                              points[i].y);
      }
    }
  }

  void TransPolygon::TransformWay(const Projection& projection,
                                  bool optimize,
                                  const std::vector<Point>& nodes)
  {
    if (nodes.empty())
    {
      length=0;
      return;
    }

    length=nodes.size();

    if (pointsSize<length)
    {
      delete [] points;
      points=new TransPoint[length];
      pointsSize=length;
    }

    if (optimize) {
      InitializeDraw();

      TransformGeoToPixel(projection,
                          nodes);

      DropSimilarPoints();

      DropRedundantPoints();

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
      start=0;
      end=nodes.size()-1;
      length=nodes.size();

      for (size_t i=0; i<nodes.size(); i++) {
        points[i].draw=true;
        projection.GeoToPixel(nodes[i].GetLon(),
                              nodes[i].GetLat(),
                              points[i].x,
                              points[i].y);
      }
    }
  }

  bool TransPolygon::GetBoundingBox(double& xmin, double& ymin,
                                    double& xmax, double& ymax) const
  {
    if (IsEmpty()) {
      return false;
    }

    xmin=points[start].x;
    xmax=points[start].x;
    ymin=points[start].y;
    ymax=points[start].y;

    for (size_t j=start+1; j<=end; j++) {
      if (points[j].draw)
      {
        xmin=std::min(xmin,points[j].x);
        xmax=std::max(xmax,points[j].x);
        ymin=std::min(ymin,points[j].y);
        ymax=std::max(ymax,points[j].y);
      }
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

  TransBuffer::TransBuffer()
  : bufferSize(131072),
    usedPoints(0),
    buffer(NULL)
  {
    buffer = new Pixel[bufferSize];
  }

  TransBuffer::~TransBuffer()
  {
    delete [] buffer;
  }

  void TransBuffer::Reset()
  {
    usedPoints=0;
  }

  void TransBuffer::AssureRoomForPoints(size_t count)
  {
    if (usedPoints+count>bufferSize)
    {
      bufferSize=bufferSize*2;

      Pixel* newBuffer=new Pixel[bufferSize];

      memcpy(newBuffer,buffer, sizeof(Pixel)*usedPoints);

      std::cout << "*** Buffer reallocation: " << bufferSize << std::endl;

      delete [] buffer;

      buffer=newBuffer;
    }
  }

  void TransBuffer::TransformArea(const Projection& projection,
                                  bool optimize,
                                  const std::vector<Point>& nodes,
                                  size_t& start, size_t &end)
  {
    transPolygon.TransformArea(projection, optimize,nodes);

    assert(!transPolygon.IsEmpty());

    AssureRoomForPoints(transPolygon.GetLength());

    start=usedPoints;
    for (size_t i=transPolygon.GetStart(); i<=transPolygon.GetEnd(); i++)
    {
      if (transPolygon.points[i].draw)
      {
        buffer[usedPoints].x=transPolygon.points[i].x;
        buffer[usedPoints].y=transPolygon.points[i].y;
        usedPoints++;
      }
    }

    end=usedPoints-1;
  }

  bool TransBuffer::TransformWay(const Projection& projection,
                    bool optimize,
                    const std::vector<Point>& nodes,
                    size_t& start, size_t &end)
  {
    transPolygon.TransformWay(projection, optimize,nodes);

    if (transPolygon.IsEmpty())
    {
      return false;
    }

    AssureRoomForPoints(transPolygon.GetLength());

    start=usedPoints;
    for (size_t i=transPolygon.GetStart(); i<=transPolygon.GetEnd(); i++)
    {
      if (transPolygon.points[i].draw)
      {
        buffer[usedPoints].x=transPolygon.points[i].x;
        buffer[usedPoints].y=transPolygon.points[i].y;
        usedPoints++;
      }
    }

    end=usedPoints-1;

    return true;
  }

  static void normalize(double x, double y, double& nx,double& ny)
  {
    double length=sqrt(x*x+y*y);

    nx=x/length;
    ny=y/length;
  }

  static double det(double x1, double y1, double x2,double y2)
  {
    return x1*y2-y1*x2;
  }

  bool TransBuffer::GenerateParallelWay(size_t orgStart, size_t orgEnd,
                                        double offset,
                                        size_t& start, size_t& end)
  {
    if (orgStart+1>orgEnd) {
      return false;
    }

    AssureRoomForPoints(orgEnd-orgStart+1);

    double oax, oay;
    double obx, oby;

    normalize(buffer[orgStart].y-buffer[orgStart+1].y,
              buffer[orgStart+1].x-buffer[orgStart].x,
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    buffer[usedPoints].x=buffer[orgStart].x+oax;
    buffer[usedPoints].y=buffer[orgStart].y+oay;

    start=usedPoints;

    usedPoints++;

    for (size_t i=orgStart+1; i<orgEnd; i++) {
      normalize(buffer[i-1].y-buffer[i].y,
                buffer[i].x-buffer[i-1].x,
                oax, oay);

      oax=offset*oax;
      oay=offset*oay;

      normalize(buffer[i].y-buffer[i+1].y,
                buffer[i+1].x-buffer[i].x,
                obx, oby);

      obx=offset*obx;
      oby=offset*oby;


      double det1=det(obx-oax, oby-oay, buffer[i+1].x-buffer[i].x, buffer[i+1].y-buffer[i].y);
      double det2=det(buffer[i].x-buffer[i-1].x, buffer[i].y-buffer[i-1].y,
                      buffer[i+1].x-buffer[i].x, buffer[i+1].y-buffer[i].y);

      if (det2>0.0001) {
        buffer[usedPoints].x=buffer[i].x+oax+det1/det2*(buffer[i].x-buffer[i-1].x);
        buffer[usedPoints].y=buffer[i].y+oay+det1/det2*(buffer[i].y-buffer[i-1].y);
      }
      else {
        buffer[usedPoints].x=buffer[i].x+oax;
        buffer[usedPoints].y=buffer[i].y+oay;
      }

      usedPoints++;
    }

    normalize(buffer[orgEnd-1].y-buffer[orgEnd].y,
              buffer[orgEnd].x-buffer[orgEnd-1].x,
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    buffer[usedPoints].x=buffer[orgEnd].x+oax;
    buffer[usedPoints].y=buffer[orgEnd].y+oay;

    end=usedPoints;

    usedPoints++;

    return true;
  }

  void TransBuffer::GetBoundingBox(size_t start, size_t end,
                                   double& xmin, double& ymin,
                                   double& xmax, double& ymax) const
  {
    xmin=buffer[start].x;
    xmax=buffer[start].x;
    ymin=buffer[start].y;
    ymax=buffer[start].y;

    for (size_t j=start+1; j<=end; j++) {
      xmin=std::min(xmin,buffer[j].x);
      xmax=std::max(xmax,buffer[j].x);
      ymin=std::min(ymin,buffer[j].y);
      ymax=std::max(ymax,buffer[j].y);
    }
  }

  void TransBuffer::GetCenterPixel(size_t start, size_t end,
                                   double& cx,
                                   double& cy) const
  {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    GetBoundingBox(start,end,xmin,ymin,xmax,ymax);

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;
  }
}
