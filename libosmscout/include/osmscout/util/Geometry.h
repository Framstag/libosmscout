#ifndef OSMSCOUT_UTIL_GEOMETRY_H
#define OSMSCOUT_UTIL_GEOMETRY_H

/*
  This source is part of the libosmscout library
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

#include <cassert>
#include <ctime>
#include <limits>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/system/Types.h>

#include <osmscout/Point.h>
#include <osmscout/util/Projection.h>

namespace osmscout {
  /**
    Returns true, if point in area.

    http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
   */
/*
  template<typename N, typename M>
  double IsLeft(const N& p0, const N& p1, const M& p2)
  {
    if (p2.id==p0.id || p2.id==p1.id) {
      return 0;
    }

    return (p1.lon-p0.lon)*(p2.lat-p0.lat)-(p2.lon-p0.lon)*(p1.lat-p0.lat);
  }

  template<typename N, typename M>
  bool IsPointInArea(const N& point,
                     const std::vector<M>& nodes)
  {
    for (int i=0; i<nodes.size()-1; i++) {
      if (point.id==nodes[i].id) {
        return true;
      }
    }

    int wn=0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<nodes.size()-1; i++) {   // edge from V[i] to V[i+1]
      if (nodes[i].lat<=point.lat) {         // start y <= P.y
        if (nodes[i+1].lat>point.lat) {     // an upward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) > 0) { // P left of edge
            ++wn;            // have a valid up intersect
          }
        }
      }
      else {                       // start y > P.y (no test needed)
        if (nodes[i+1].lat<=point.lat) {    // a downward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) < 0) { // P right of edge
            --wn;            // have a valid down intersect
          }
        }
      }
    }

    return wn!=0;
  }*/

  /**
    Returns true, if point in area.

    See http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    */

  template<typename N, typename M>
  bool IsPointInArea(const N& point,
                     const std::vector<M>& nodes)
  {
    int  i,j;
    bool c=false;

    for (i=0, j=nodes.size()-1; i<(int)nodes.size(); j=i++) {
      if (point.GetId()==nodes[i].GetId()) {
        return true;
      }

      if ((nodes[i].GetLat()>point.GetLat())!=(nodes[j].GetLat()>point.GetLat()) &&
          (point.GetLon()<(nodes[j].GetLon()-nodes[i].GetLon())*(point.GetLat()-nodes[i].GetLat()) /
           (nodes[j].GetLat()-nodes[i].GetLat())+nodes[i].GetLon()))  {
        c=!c;
      }
    }

    return c;
  }

  /**
    Gives information about the position of the point in relation to the area.

    If -1 returned, the point is outside the area, if 0, the point is on the area boundary, 1
    the point is within the area.
   */
  template<typename N, typename M>
  int GetRelationOfPointToArea(const N& point,
                               const std::vector<M>& nodes)
  {
    int  i,j;
    bool c=false;

    for (i=0, j=nodes.size()-1; i<(int)nodes.size(); j=i++) {
      if (point.id==nodes[i].GetId()) {
        return 0;
      }

      if ((nodes[i].lat>point.lat)!=(nodes[j].lat>point.lat) &&
          (point.lon<(nodes[j].lon-nodes[i].lon)*(point.lat-nodes[i].lat) /
           (nodes[j].lat-nodes[i].lat)+nodes[i].lon))  {
        c=!c;
      }
    }

    return c ? 1 : -1;
  }

  /**
    Return true, if area a is in area b
    */
  template<typename N,typename M>
  bool IsAreaInArea(const std::vector<N>& a,
                    const std::vector<M>& b)
  {
    for (typename std::vector<N>::const_iterator i=a.begin(); i!=a.end(); i++) {
      if (!IsPointInArea(*i,b)) {
        return false;
      }
    }

    return true;
  }

  /**
    Returns true, if area a is completely in area b under the assumption that the area a is either
    completely within or outside the area b.
    */
  template<typename N,typename M>
  bool IsAreaSubOfArea(const std::vector<N>& a,
                       const std::vector<M>& b)
  {
    for (typename std::vector<N>::const_iterator i=a.begin(); i!=a.end(); i++) {
      int relPos=GetRelationOfPointToArea(*i,b);

      if (relPos>0) {
        return true;
      }
      else if (relPos<0) {
        return false;
      }
    }

    return false;
  }

  struct OSMSCOUT_API Coord
  {
    size_t x;
    size_t y;

    Coord(size_t x, size_t y)
     :x(x),y(y)
    {
      // no code
    }

    bool operator==(const Coord& other) const
    {
      return x==other.x && y==other.y;
    }

    bool operator<(const Coord& other) const
    {
      return y<other.y ||
      ( y==other.y && x<other.x);
    }
  };

  extern OSMSCOUT_API double Log2(double x);
  extern OSMSCOUT_API size_t Pow(size_t a, size_t b);
  extern OSMSCOUT_API double GetSphericalDistance(double aLon, double aLat,
                                                  double bLon, double bLat);
  extern OSMSCOUT_API double GetEllipsoidalDistance(double aLon, double aLat,
                                                   double bLon, double bLat);

  struct OSMSCOUT_API ScanCell
  {
    int x;
    int y;

    ScanCell(int x, int y);
  };

  void OSMSCOUT_API ScanConvertLine(int x1, int y1,
                                    int x2, int y2,
                                    std::vector<ScanCell>& cells);
  void OSMSCOUT_API ScanConvertLine(const std::vector<Point>& points,
                                    double xTrans, double cellWidth,
                                    double yTrans, double cellHeight,
                                    std::vector<ScanCell>& cells);
}

#endif
