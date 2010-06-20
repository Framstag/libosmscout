/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/MapPainter.h>

#include <cmath>

namespace osmscout {

  static const double gradtorad=2*M_PI/360;
  static size_t optimizeLimit=512;
  static double relevantPosDeriviation=2.0;
  static double relevantSlopeDeriviation=0.1;

  MapParameter::MapParameter()
  {
    // no code
  }  
  
  MapParameter::~MapParameter()
  {
    // no code
  }
  
  MapPainter::MapPainter()
  {
    // no code
  }

  MapPainter::~MapPainter()
  {
    // no code
  }
  
  bool MapPainter::IsVisible(const Projection& projection,
                             const std::vector<Point>& nodes) const
  {
    if (nodes.size()==0) {
      return false;
    }

    // Bounding box
    double lonMin=nodes[0].lon;
    double lonMax=nodes[0].lon;
    double latMin=nodes[0].lat;
    double latMax=nodes[0].lat;

    for (size_t i=1; i<nodes.size(); i++) {
      lonMin=std::min(lonMin,nodes[i].lon);
      lonMax=std::max(lonMax,nodes[i].lon);
      latMin=std::min(latMin,nodes[i].lat);
      latMax=std::max(latMax,nodes[i].lat);
    }

    // If bounding box is neither left or right nor above or below
    // it must somehow cover the map area.
    return projection.GeoIsIn(lonMin,latMin,lonMax,latMax);
  }

  void MapPainter::TransformArea(const Projection& projection,
                                 const std::vector<Point>& nodes)
  {
    if (projection.GetMagnification()>optimizeLimit) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
      }
    }
    else {
      drawNode[0]=true;
      drawNode[nodes.size()-1]=true;

      // Drop every point that is on direct line between two points A and B
      for (size_t i=1; i+1<nodes.size(); i++) {
        drawNode[i]=std::abs((nodes[i].lon-nodes[i-1].lon)/
                             (nodes[i].lat-nodes[i-1].lat)-
                             (nodes[i+1].lon-nodes[i].lon)/
                             (nodes[i+1].lat-nodes[i].lat))>=relevantSlopeDeriviation;
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      for (size_t i=1; i<nodes.size()-1; i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (!drawNode[j]) {
            j++;
          }

          if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
              std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
            drawNode[i]=false;
          }
        }
      }
    }
  }

  void MapPainter::TransformWay(const Projection& projection,
                                const std::vector<Point>& nodes)
  {
    if (projection.GetMagnification()>optimizeLimit) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
      }
    }
    else {
      size_t a;

      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
      }

      if (nodes.size()>=3) {
        a=0;
        while (a+1<nodes.size()) {
          if (projection.GeoIsIn(nodes[a].lon,nodes[a].lat)) {
            break;
          }

          a++;
        }

        if (a>1) {
          for (size_t i=0; i<a-1; i++) {
            drawNode[i]=false;
          }
        }
      }

      if (nodes.size()>=3) {
        a=nodes.size()-1;
        while (a>0) {
          if (projection.GeoIsIn(nodes[a].lon,nodes[a].lat)) {
            break;
          }

          a--;
        }

        if (a<nodes.size()-2) {
          for (size_t i=a+2; i<nodes.size(); i++) {
            drawNode[i]=false;
          }
        }
      }

      // Drop every point that is on direct line between two points A and B
      for (size_t i=0; i+2<nodes.size(); i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (j<nodes.size() && !drawNode[j]) {
            j++;
          }

          size_t k=j+1;
          while (k<nodes.size() && !drawNode[k]) {
            k++;
          }

          if (j<nodes.size() && k<nodes.size()) {
            drawNode[j]=std::abs((nodes[j].lon-nodes[i].lon)/
                               (nodes[j].lat-nodes[i].lat)-
                               (nodes[k].lon-nodes[j].lon)/
                               (nodes[k].lat-nodes[j].lat))>=relevantSlopeDeriviation;
          }
        }
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      if (nodes.size()>2) {
        for (size_t i=1; i<nodes.size()-1; i++) {
          if (drawNode[i]) {
            size_t j=i+1;

            while (j+1<nodes.size() &&
                   !drawNode[j]) {
              j++;
            }

            if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
                std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
              drawNode[i]=false;
            }
          }
        }
      }

      /*
      // Check which nodes or not visible in the given way
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (way->nodes[i].lon<lonMin || way->nodes[i].lon>lonMax ||
            way->nodes[i].lat<latMin || way->nodes[i].lat>latMax){
          outNode[i]=true;
        }
      }

      if (outNode[1]) {
        drawNode[0]=false;
      }

      for (size_t i=1; i<way->nodes.size()-1; i++) {
        if (outNode[i-1] && outNode[i+1]) {
          drawNode[i]=false;
        }
      }

      if (outNode[way->nodes.size()-2]) {
        drawNode[way->nodes.size()-1]=false;
      }*/
    }
  }

  bool MapPainter::GetBoundingBox(const std::vector<Point>& nodes,
                                  double& xmin, double& ymin,
                                  double& xmax, double& ymax)
  {
    if (nodes.size()==0) {
      return false;
    }

    xmin=nodes[0].lon;
    xmax=nodes[0].lon;
    ymin=nodes[0].lat;
    ymax=nodes[0].lat;

    for (size_t j=1; j<nodes.size(); j++) {
      xmin=std::min(xmin,nodes[j].lon);
      xmax=std::max(xmax,nodes[j].lon);
      ymin=std::min(ymin,nodes[j].lat);
      ymax=std::max(ymax,nodes[j].lat);
    }

    return true;
  }

  bool MapPainter::GetCenterPixel(const Projection& projection,
                                  const std::vector<Point>& nodes,
                                  double& cx,
                                  double& cy)
  {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    if (!GetBoundingBox(nodes,xmin,ymin,xmax,ymax)) {
      return false;
    }

    projection.GeoToPixel(xmin,ymin,xmin,ymin);
    projection.GeoToPixel(xmax,ymax,xmax,ymax);

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;

    return true;
  }
}

