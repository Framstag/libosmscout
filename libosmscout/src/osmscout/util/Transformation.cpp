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

#include <cmath>
#include <limits>

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
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              points[i].x,points[i].y);
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
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              points[i].x,points[i].y);
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
        projection.GeoToPixel(nodes[i].lon,
                              nodes[i].lat,
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
}
