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

#include <algorithm>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/system/Types.h>

#include <osmscout/Point.h>

namespace osmscout {
  /**
   * Returns true, if the lines defined by the given coordinates intersect.
   */
  template<typename N>
  bool LinesIntersect(const N& a1,
                      const N& a2,
                      const N& b1,
                      const N& b2)
  {
    double ua_numr=(b2.x-b1.x)*(a1.y-b1.y)-(b2.y-b1.y)*(a1.x-b1.x);
    double ub_numr=(a2.x-a1.x)*(a1.y-b1.y)-(a2.y-a1.y)*(a1.x-b1.x);
    double denr=(b2.y-b1.y)*(a2.x-a1.x)-(b2.x-b1.x)*(a2.y-a1.y);

    if(denr==0.0) {
      // lines are coincident
      return ua_numr==0.0 &&
             ub_numr==0.0;
    }

    double ua=ua_numr/denr;
    double ub=ub_numr/denr;

    return ua>=0.0 &&
           ua<=1.0 &&
           ub>=0.0 &&
           ub<=1.0;
  }


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
      if (point.GetId()==nodes[i].GetId()) {
        return 0;
      }

      if ((nodes[i].GetLat()>point.GetLat())!=(nodes[j].GetLat()>point.GetLat()) &&
          (point.GetLon()<(nodes[j].GetLon()-nodes[i].GetLon())*(point.GetLat()-nodes[i].GetLat()) /
           (nodes[j].GetLat()-nodes[i].GetLat())+nodes[i].GetLon()))  {
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


  /**
   * Returns true, if the handed polygons are simple (aka not complex),
   * that means, we have no intersections.
   */
  template<typename N>
  bool AreaIsSimple(const std::vector<std::pair<N,N> >& edges,
                    const std::vector<bool>& edgeStartsNewPoly)
  {
    size_t edgesIntersect=0;

    for (size_t i=0; i<edges.size(); i++) {
      edgesIntersect=0;

      for (size_t j=i+1; j<edges.size(); j++) {
        if (LinesIntersect(edges[i].first,
                           edges[i].second,
                           edges[j].first,
                           edges[j].second)) {
          edgesIntersect++;

          // we check the first edge of a sole polygon against every
          // other edge and expect to see 2 intersections for
          // adjacent edges; polygon is complex if there are more
          // intersections
          if (edgeStartsNewPoly[i]) {
            if (edgesIntersect>2) {
              return false;
            }
          }

          // otherwise we check an edge that isn't the first
          // edge against every other edge excluding those that
          // have already been tested (this means one adjacent
          // edge); polygon is complex if there is more than one
          // intersection
          else {
            if (edgesIntersect>1) {
              return false;
            }
          }
        }
      }
    }

    return true;
  }

  /**
   *  Returns true, if the polygon is counter clock wise (CCW)
   */
  template<typename N>
  bool AreaIsCCW(const std::vector<N> &edges)
  {
    // based on http://en.wikipedia.org/wiki/Curve_orientation
    // and http://local.wasp.uwa.edu.au/~pbourke/geometry/clockwise/

    // note: polygon must be simple
    // note: this assumes 2d cartesian coordinate space is used;
    // for geographic, expect Vec2.x=lon and Vec2.y=lat!

    int ptIdx=0;

    for (int i=1; i<edges.size(); i++) {
      // find the point with the smallest y value,
      if (edges[i].y<edges[ptIdx].y) {
        ptIdx=i;
      }
      // if y values are equal save the point with greatest x
      else if (edges[i].y==edges[ptIdx].y) {
        if (edges[i].x<edges[ptIdx].x) {
          ptIdx=i;
        }
      }
    }

    int prevIdx=(ptIdx==0) ? edges.size()-1 : ptIdx-1;
    int nextIdx=(ptIdx==edges.size()-1) ? 0 : ptIdx+1;

    double signedArea=(edges[ptIdx].x-edges[prevIdx].x)*
                      (edges[nextIdx].y-edges[ptIdx].y)-
                      (edges[ptIdx].y-edges[prevIdx].y)*
                      (edges[nextIdx].x-edges[ptIdx].x);

    return signedArea>0.0;
  }

  template<typename N>
  bool AreaIsValid(std::vector<N>& outerPoints,
                   std::vector<std::vector<N> >& innerPoints)
  {
    if (outerPoints.size()<3) {
      return false;
    }

    size_t numEdges=outerPoints.size();

    for (size_t i=0; i<innerPoints.size(); i++) {
      numEdges+=innerPoints[i].size();
    }

    std::vector<bool>            edgeStartsNewPoly(numEdges,false);
    std::vector<std::pair<N,N> > listEdges(numEdges);
    size_t                       cEdge=0;

    // temporarily wrap around vertices
    // (first == last) to generate edge lists
    outerPoints.push_back(outerPoints[0]);
    for (size_t i=0; i<innerPoints.size(); i++) {
      innerPoints[i].push_back(innerPoints[i][0]);
    }

    // outer poly
    edgeStartsNewPoly[0]=true;
    for (size_t i=1; i<outerPoints.size(); i++) {
      std::pair<N, N> outerEdge;

      outerEdge.first=outerPoints[i-1];
      outerEdge.second=outerPoints[i];
      listEdges[cEdge]=outerEdge;

      cEdge++;
    }

    // inner polys
    for (size_t i=0; i<innerPoints.size(); i++) {
      edgeStartsNewPoly[cEdge]=true;

      for (size_t j=1; j<innerPoints[i].size(); j++) {
        std::pair<N, N> innerEdge;

        innerEdge.first=innerPoints[i][j-1];
        innerEdge.second=innerPoints[i][j];
        listEdges[cEdge]=innerEdge;

        cEdge++;
      }
    }

    // revert vertex list modifications (not
    // really the 'nicest' way of doing this)
    outerPoints.pop_back();
    for (size_t i=0; i<innerPoints.size(); i++) {
      innerPoints[i].pop_back();
    }

    if (AreaIsSimple(listEdges,edgeStartsNewPoly))
        {
      // expect listOuterPts to be CCW and innerPts
      // to be CW, if not then reverse point order

      if (!AreaIsCCW(outerPoints)) {
        std::reverse(outerPoints.begin(),
                     outerPoints.end());
      }

      for (int i=0; i<innerPoints.size(); i++) {
        if (AreaIsCCW(innerPoints[i])) {
          std::reverse(innerPoints[i].begin(),
                       innerPoints[i].end());
        }
      }
    }
    else {
      return false;
    }

    return true;
  }

  template<typename N>
  bool AreaIsValid(std::vector<N> &outerPoints)
  {
    std::vector<std::vector<N> > listListInnerPts; //empty

    return AreaIsValid(outerPoints,listListInnerPts);
  }

  struct OSMSCOUT_API Coord
  {
    uint32_t x;
    uint32_t y;

    Coord(uint32_t x, uint32_t y)
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

  extern OSMSCOUT_API size_t Pow(size_t a, size_t b);
  extern OSMSCOUT_API double GetSphericalDistance(double aLon, double aLat,
                                                  double bLon, double bLat);
  extern OSMSCOUT_API double GetEllipsoidalDistance(double aLon, double aLat,
                                                   double bLon, double bLat);

  extern OSMSCOUT_API double GetSphericalBearingInitial(double aLon, double aLat,
                                                        double bLon, double bLat);

  extern OSMSCOUT_API double GetSphericalBearingFinal(double aLon, double aLat,
                                                      double bLon, double bLat);

  extern OSMSCOUT_API double NormalizeRelativeAngel(double angle);

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
