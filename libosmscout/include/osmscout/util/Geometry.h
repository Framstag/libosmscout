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
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>
#include <osmscout/system/Types.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>
#include <osmscout/PointSequence.h>
#include <osmscout/Types.h>

#include <osmscout/util/GeoBox.h>

namespace osmscout { 

  /**
   * \defgroup Geometry Geometric helper
   *
   * Collection of classes and methods releated to low level geometric
   * stuff.
   */

  /**
   * \ingroup Geometry
   * Calculate the bounding box of the (non empty) vector of geo coords
   *
   * @param nodes
   *    The geo coordinates
   * @param minLon
   * @param maxLon
   * @param minLat
   * @param maxLat
   */
  template<typename N>
  void GetBoundingBox(const std::vector<N>& nodes,
                      double& minLon,
                      double& maxLon,
                      double& minLat,
                      double& maxLat)
  {
    assert(!nodes.empty());

    minLon=nodes[0].GetLon();
    maxLon=nodes[0].GetLon();
    minLat=nodes[0].GetLat();
    maxLat=nodes[0].GetLat();

    for (size_t i=1; i<nodes.size(); i++) {
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLon=std::max(maxLon,nodes[i].GetLon());
      minLat=std::min(minLat,nodes[i].GetLat());
      maxLat=std::max(maxLat,nodes[i].GetLat());
    }
  }

  /**
   * \ingroup Geometry
   * Calculate the bounding box of the (non empty) vector of geo coords
   *
   * @param nodes
   *    The geo coordinates
   * @param minLon
   * @param maxLon
   * @param minLat
   * @param maxLat
   */
  template<typename N>
  void GetBoundingBox(const std::vector<N>& nodes,
                      GeoBox& boundingBox)
  {
    assert(!nodes.empty());

    double minLon=nodes[0].GetLon();
    double maxLon=nodes[0].GetLon();
    double minLat=nodes[0].GetLat();
    double maxLat=nodes[0].GetLat();

    for (size_t i=1; i<nodes.size(); i++) {
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLon=std::max(maxLon,nodes[i].GetLon());
      minLat=std::min(minLat,nodes[i].GetLat());
      maxLat=std::max(maxLat,nodes[i].GetLat());
    }

    boundingBox.Set(GeoCoord(minLat,minLon),
                    GeoCoord(maxLat,maxLon));
  }

  /**
   * \ingroup Geometry
   * Calculate the bounding box of the (non empty) vector of geo coords
   *
   * @param nodes
   *    The geo coordinates
   * @param minLon
   * @param maxLon
   * @param minLat
   * @param maxLat
   */
  inline void GetBoundingBox(const PointSequence& nodes,
                      GeoBox& boundingBox)
  {
    assert(!nodes.empty());

    const GeoBox bbox = nodes.bbox();
    
    boundingBox.Set(bbox.GetMinCoord(), bbox.GetMaxCoord());
  }

  /**
   * \ingroup Geometry
   *
   * Returns true, if the lines defined by the given coordinates intersect.
   */
  template<typename N>
  bool LinesIntersect(const N& a1,
                      const N& a2,
                      const N& b1,
                      const N& b2)
  {
    if (a1.IsEqual(b1) ||
        a1.IsEqual(b2) ||
        a2.IsEqual(b1) ||
        a2.IsEqual(b2)) {
      return true;
    }

    double denr=(b2.GetLat()-b1.GetLat())*(a2.GetLon()-a1.GetLon())-
                (b2.GetLon()-b1.GetLon())*(a2.GetLat()-a1.GetLat());

    double ua_numr=(b2.GetLon()-b1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (b2.GetLat()-b1.GetLat())*(a1.GetLon()-b1.GetLon());
    double ub_numr=(a2.GetLon()-a1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (a2.GetLat()-a1.GetLat())*(a1.GetLon()-b1.GetLon());

    if (denr==0.0) {
      if (ua_numr==0.0 && ub_numr==0.0) {
        // This gives currently false hits because of number resolution problems, if two lines are very
        // close together and for example are part of a very details node curve intersections are detected.
        return true;
      }
      else {
        return false;
      }
    }

    double ua=ua_numr/denr;
    double ub=ub_numr/denr;

    return ua>=0.0 &&
           ua<=1.0 &&
           ub>=0.0 &&
           ub<=1.0;
  }

  /**
   * \ingroup Geometry
   *
   * Returns true, if the lines defined by the given coordinates intersect. Returns the intersection.
   */
  template<typename N>
  bool GetLineIntersection(const N& a1,
                           const N& a2,
                           const N& b1,
                           const N& b2,
                           N& intersection)
  {
    if (a1.IsEqual(b1) ||
        a1.IsEqual(b2) ||
        a2.IsEqual(b1) ||
        a2.IsEqual(b2)) {
      return true;
    }

    double denr=(b2.GetLat()-b1.GetLat())*(a2.GetLon()-a1.GetLon())-
                (b2.GetLon()-b1.GetLon())*(a2.GetLat()-a1.GetLat());

    double ua_numr=(b2.GetLon()-b1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (b2.GetLat()-b1.GetLat())*(a1.GetLon()-b1.GetLon());
    double ub_numr=(a2.GetLon()-a1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (a2.GetLat()-a1.GetLat())*(a1.GetLon()-b1.GetLon());

    if (denr==0.0) {
      if (ua_numr==0.0 && ub_numr==0.0) {
        // This gives currently false hits because of number resolution problems, if two lines are very
        // close together and for example are part of a very details node curve intersections are detected.
        return true;
      }
      else {
        return false;
      }
    }

    double ua=ua_numr/denr;
    double ub=ub_numr/denr;

    if (ua>=0.0 &&
        ua<=1.0 &&
        ub>=0.0 &&
        ub<=1.0) {
      intersection.Set(a1.GetLat()+ua*(a2.GetLat()-a1.GetLat()),
                       a1.GetLon()+ua*(a2.GetLon()-a1.GetLon()));
      return true;
    }

    return false;
  }


  template<typename N>
  double DistanceSquare(const N& a,
                        const N& b)
  {
    double lonDelta=a.GetLon()-b.GetLon();
    double latDelta=a.GetLat()-b.GetLat();

    return lonDelta*lonDelta+latDelta*latDelta;
  }

  /**
   * \ingroup Geometry
   *
   * Returns true, if point in area.
   *
   * http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm
   */
/*
  template<typename N, typename M>
  inline double IsLeft(const N& p0,
                       const N& p1,
                       const M& p2)
  {
    if ((p2.GetLat()==p0.GetLat() &&
         p2.GetLon()==p0.GetLon()) ||
        (p2.GetLat()==p1.GetLat() &&
         p2.GetLon()==p1.GetLon())) {
      return 0;
    }

    return (p1.GetLon()-p0.GetLon())*(p2.GetLat()-p0.GetLat())-
           (p2.GetLon()-p0.GetLon())*(p1.GetLat()-p0.GetLat());
  }

  template<typename N, typename M>
  inline bool IsCoordInArea(const N& point,
                            const std::vector<M>& nodes)
  {
    for (int i=0; i<(int)nodes.size()-1; i++) {
      if (point.GetLat()==nodes[i].GetLat() &&
          point.GetLon()==nodes[i].GetLon()) {
        return true;
      }
    }

    int wn=0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<(int)nodes.size()-1; i++) {   // edge from V[i] to V[i+1]
      if (nodes[i].GetLat()<=point.GetLat()) {         // start y <= P.y
        if (nodes[i+1].GetLat()>point.GetLat()) {     // an upward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) > 0) { // P left of edge
            ++wn;            // have a valid up intersect
          }
        }
      }
      else {                       // start y > P.y (no test needed)
        if (nodes[i+1].GetLat()<=point.GetLat()) {    // a downward crossing
          if (IsLeft(nodes[i],nodes[i+1],point) < 0) { // P right of edge
            --wn;            // have a valid down intersect
          }
        }
      }
    }

    return wn!=0;
  }*/
  
  /**
   * \ingroup Geometry
   * Gives information about the position of the point in relation to the area.
   *
   * If -1 returned, the point is outside the area, if 0, the point is on the area boundary, 1
   * the point is within the area.
   */
  template<typename N,typename M>
  inline int GetRelationOfPointToArea(const N& point,
                                      const std::vector<M>& nodes)
  {
    size_t i,j;
    bool   c=false;

    for (i=0, j=nodes.size()-1; i<nodes.size(); j=i++) {
      if (point.GetLat()==nodes[i].GetLat() &&
          point.GetLon()==nodes[i].GetLon()) {
        return 0;
      }

      if ((((nodes[i].GetLat()<=point.GetLat()) && (point.GetLat()<nodes[j].GetLat())) ||
           ((nodes[j].GetLat()<=point.GetLat()) && (point.GetLat()<nodes[i].GetLat()))) &&
          (point.GetLon()<(nodes[j].GetLon()-nodes[i].GetLon())*(point.GetLat()-nodes[i].GetLat())/(nodes[j].GetLat()-nodes[i].GetLat())+
           nodes[i].GetLon())) {
        c=!c;
      }
    }

    return c ? 1 : -1;
  }
  
  template<typename N>
  inline int GetRelationOfPointToArea(const N& point,
                                      const PointSequence& nodes)
  {
    bool   c=false;

    PointSequenceIterator it = nodes.begin();
    Point a = *it;
    Point b = nodes[nodes.size() -1];
    
    for (;  it != nodes.end(); b = a, ++it) {
      
      if (point.GetLat()==a.GetLat() &&
          point.GetLon()==a.GetLon()) {
        return 0;
      }

      if ((((a.GetLat()<=point.GetLat()) && (point.GetLat()<b.GetLat())) ||
           ((b.GetLat()<=point.GetLat()) && (point.GetLat()<a.GetLat()))) &&
          (point.GetLon()<(b.GetLon()-a.GetLon())*(point.GetLat()-a.GetLat())/(b.GetLat()-a.GetLat())+
           a.GetLon())) {
        c=!c;
      }
    }

    return c ? 1 : -1;
  }
  

  /**
   * \ingroup Geometry
   *
   * Returns true, if point in on the area border or within the area.
   *
   * See http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
   */
  template<typename N, typename M>
  inline bool IsCoordInArea(const N& point,
                            const std::vector<M>& nodes)
  {
    return GetRelationOfPointToArea(point, nodes) >= 0;
  }
  
  template<typename N>
  inline bool IsCoordInArea(const N& point,
                            const PointSequence& nodes)
  {
    return GetRelationOfPointToArea(point, nodes) >= 0;
  }

  /**
   * \ingroup Geometry
   * Return true, if area a is completely in area b
   */
  template<typename N,typename M>
  inline bool IsAreaCompletelyInArea(const std::vector<N>& a,
                                     const std::vector<M>& b)
  {
    for (const auto& node : a) {
      if (GetRelationOfPointToArea(node,b)<0) {
        return false;
      }
    }

    return true;
  }

  /**
   * \ingroup Geometry
   * Return true, if area a is completely in area b
   */
  template<typename M>
  inline bool IsAreaCompletelyInArea(const PointSequence& a,
                                     const std::vector<M>& b)
  {
    for (const auto& node : a) {
      if (GetRelationOfPointToArea(node,b)<0) {
        return false;
      }
    }

    return true;
  }

  /**
   * \ingroup Geometry
   * Return true, if at least one point of area a in within area b
   */
  template<typename N,typename M>
  inline bool IsAreaAtLeastPartlyInArea(const std::vector<N>& a,
                                        const std::vector<M>& b)
  {
    for (const auto& node : a) {
      if (GetRelationOfPointToArea(node,b)>=0) {
        return true;
      }
    }

    return false;
  }

  /**
   * \ingroup Geometry
   * Return true, if at least one point of area a in within area b
   */
  template<typename M>
  inline bool IsAreaAtLeastPartlyInArea(const PointSequence& a,
                                        const std::vector<M>& b)
  {
    for (const auto& node : a) {
      if (GetRelationOfPointToArea(node,b)>=0) {
        return true;
      }
    }

    return false;
  }

  /**
   * \ingroup Geometry
   * Return true, if at least one point of area a in within area b
   */
  template<typename N>
  inline bool IsAreaAtLeastPartlyInArea(const std::vector<N>& a,
                                        const PointSequence& b)
  {
    for (const auto& node : a) {
      if (GetRelationOfPointToArea(node,b)>=0) {
        return true;
      }
    }

    return false;
  }

  /**
   * \ingroup Geometry
   * Assumes that the given areas do not intersect.
   *
   * Returns true, of area a is within b (because at least
   * one point of area a is in b), else (at least one point
   * of area a is outside area b) false
   */
  template<typename N,typename M>
  inline bool IsAreaSubOfArea(const std::vector<N>& a,
                              const std::vector<M>& b)
  {
    for (const auto& node : a) {
      int relPos=GetRelationOfPointToArea(node,b);

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
   * \ingroup Geometry
   * Assumes that the given areas do not intersect.
   *
   * Returns true, of area a is within b (because at least
   * one point of area a is in b), else (at least one point
   * of area a is outside area b) false
   */
  inline bool IsAreaSubOfArea(const PointSequence& a,
                              const PointSequence& b)
  {    
    for (const auto& node : a) {
      int relPos=GetRelationOfPointToArea(node,b);

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
   * \ingroup Geometry
   * Assumes that the given areas do not intersect.
   *
   * Returns true, of area a is within b. This version uses some heuristic
   * based on the assumption that areas are either in another area or not - but
   * there may be some smaller errors due to areas slightly overlapping.
   */
  template<typename N,typename M>
  inline bool IsAreaSubOfAreaQuorum(const std::vector<N>& a,
                                    const std::vector<M>& b)
  {
    size_t pro=0;
    size_t contra=0;
    size_t count=0;

    for (const auto& node : a) {
      int relPos=GetRelationOfPointToArea(node,b);

      ++count;

      if (relPos>0) {
        ++pro;
      }
      else if (relPos<0) {
        ++contra;
      }

      if (count>=100 && pro/20>contra) {
        return true;
      }
    }

    return pro/20>contra;
  }

  /**
   * \ingroup Geometry
   * Returns true, if the handed polygon is simple (aka not complex).
   *
   * Currently the following checks are done:
   * + Polygon has at least 3 points
   * + Assure that the line segments that make up the polygon
   *   only meet at their end points.
   */
  template<typename N>
  bool AreaIsSimple(std::vector<N> points)
  {
    if (points.size()<3) {
      return false;
    }

    points.push_back(points[0]);

    size_t edgesIntersect=0;

    for (size_t i=0; i<points.size()-1; i++) {
      edgesIntersect=0;

      for (size_t j=i+1; j<points.size()-1; j++) {
        if (LinesIntersect(points[i],
                           points[i+1],
                           points[j],
                           points[j+1])) {
          edgesIntersect++;

          if (i==0) {
            if (edgesIntersect>2) {
              return false;
            }
          }
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
   * \ingroup Geometry
   * Returns true, if the handed polygons are simple (aka not complex).
   * This method supports passing multiple closed polygons and checks
   * all of them.
   *
   * Currently it is checked, that the line segments that make up the
   * polygon, only meet at their end points.
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
   * \ingroup Geometry
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

  /**
   * \ingroup Geometry
   *
   * @param outerPoints
   * @param innerPoints
   * @return
   */
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

  /**
   * \ingroup Geometry
   *
   */
  inline void Normalize(double x,
                        double y,
                        double& nx,
                        double& ny)
  {
    double length=sqrt(x*x+y*y);

    nx=x/length;
    ny=y/length;
  }

  /**
   * \ingroup Geometry
   * Calculates the determinant of the line between the given points.
   */
  inline double Det(double x1,
                    double y1,
                    double x2,
                    double y2)
  {
    return x1*y2-y1*x2;
  }


  /**
   * \ingroup Math
   *
   */
  extern OSMSCOUT_API size_t Pow(size_t a, size_t b);

  /**
   * \ingroup Geometry
   * Returns true, if the closed polygon with the given nodes is oriented clockwise.
   *
   * It is assumed, that the polygon is valid. Validity is not checked.
   *
   * See http://en.wikipedia.org/wiki/Curve_orientation.
   */
  template<typename N>
  bool AreaIsClockwise(const std::vector<N>& edges)
  {
    assert(edges.size()>=3);
    // based on http://en.wikipedia.org/wiki/Curve_orientation
    // and http://local.wasp.uwa.edu.au/~pbourke/geometry/clockwise/

    // note: polygon must be simple

    size_t ptIdx=0;

    for (size_t i=1; i<edges.size(); i++) {
      // find the point with the smallest y value,
      if (edges[i].GetLat()<edges[ptIdx].GetLat()) {
        ptIdx=i;
      }
        // if y values are equal save the point with greatest x
      else if (edges[i].GetLat()==edges[ptIdx].GetLat()) {
        if (edges[i].GetLon()<edges[ptIdx].GetLon()) {
          ptIdx=i;
        }
      }
    }

    size_t prevIdx=(ptIdx==0) ? edges.size()-1 : ptIdx-1;
    size_t nextIdx=(ptIdx==edges.size()-1) ? 0 : ptIdx+1;

    double signedArea=(edges[ptIdx].GetLon()-edges[prevIdx].GetLon())*
                      (edges[nextIdx].GetLat()-edges[ptIdx].GetLat())-
                      (edges[ptIdx].GetLat()-edges[prevIdx].GetLat())*
                      (edges[nextIdx].GetLon()-edges[ptIdx].GetLon());

    return signedArea<0.0;
  }


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
  template<typename N>
  double CalculateDistancePointToLineSegment(const N& p,
                                             const N& a,
                                             const N& b)
  {
    double xdelta=b.GetLon()-a.GetLon();
    double ydelta=b.GetLat()-a.GetLat();

    if (xdelta==0 && ydelta==0) {
      return std::numeric_limits<double>::infinity();
    }

    double u=((p.GetLon()-a.GetLon())*xdelta+(p.GetLat()-a.GetLat())*ydelta)/(xdelta*xdelta+ydelta*ydelta);

    double cx,cy;

    if (u<0) {
      cx=a.GetLon();
      cy=a.GetLat();
    }
    else if (u>1) {
      cx=b.GetLon();
      cy=b.GetLat();
    }
    else {
      cx=a.GetLon()+u*xdelta;
      cy=a.GetLat()+u*ydelta;
    }

    double dx=cx-p.GetLon();
    double dy=cy-p.GetLat();

    return sqrt(dx*dx+dy*dy);
  }

  extern OSMSCOUT_API double CalculateDistancePointToLineSegment(const GeoCoord& p,
                                                                 const GeoCoord& a,
                                                                 const GeoCoord& b,
                                                                 GeoCoord& intersection);

  /**
   * \ingroup Geometry
   * Calculates the spherical distance between the two given points
   * on the sphere.
   */
  extern OSMSCOUT_API double GetSphericalDistance(double aLon, double aLat,
                                                  double bLon, double bLat);

  /**
   * \ingroup Geometry
   * Calculates the spherical distance between the two given points
   * on the sphere.
   */
  extern OSMSCOUT_API double GetSphericalDistance(const GeoCoord& a,
                                                  const GeoCoord& b);

  /**
   * \ingroup Geometry
   * Calculates the ellipsoidal (WGS-84) distance between the two given points
   * on the ellipsoid.
   */
  extern OSMSCOUT_API double GetEllipsoidalDistance(double aLon, double aLat,
                                                   double bLon, double bLat);

  /**
   * \ingroup Geometry
   * Calculates the ellipsoidal (WGS-84) distance between the two given points
   * on the ellipsoid.
   */
  extern OSMSCOUT_API double GetEllipsoidalDistance(const GeoCoord& a,
                                                    const GeoCoord& b);

  /**
   * \ingroup Geometry
   * Given a starting point and a bearing and a distance calculates the
   * coordinates of the resulting point in the (WGS-84) ellipsoid.
   */
  extern OSMSCOUT_API void GetEllipsoidalDistance(double lat1, double lon1,
                                                  double bearing, double distance,
                                                  double& lat2, double& lon2);

  /**
   * \ingroup Geometry
   *Calculates the initial bearing for a line from one coordinate two the other coordinate
   *on a sphere.
   */
  extern OSMSCOUT_API double GetSphericalBearingInitial(double aLon, double aLat,
                                                        double bLon, double bLat);

  /**
   * \ingroup Geometry
   *Calculates the initial bearing for a line from one coordinate two the other coordinate
   *on a sphere.
   */
  extern OSMSCOUT_API double GetSphericalBearingInitial(const GeoCoord& a,
                                                        const GeoCoord& b);

  /**
   * \ingroup Geometry
   *Calculates the final bearing for a line from one coordinate two the other coordinate
   *on a sphere.
   */
  extern OSMSCOUT_API double GetSphericalBearingFinal(double aLon, double aLat,
                                                      double bLon, double bLat);

  /**
   * COnvert the bearing to to a direction description in releation tothe compass.
   */
  extern OSMSCOUT_API std::string BearingDisplayString(double bearing);

  /**
   * \ingroup Geometry
   * Normalizes the given bearing to be in the interval [-180.0 - 180.0]
   */
  extern OSMSCOUT_API double NormalizeRelativeAngel(double angle);

  struct OSMSCOUT_API ScanCell
  {
    int x;
    int y;

    ScanCell(int x, int y);
  };

  /**
   * \ingroup Geometry
   * Does a scan conversion for a line between the given coordinates.
   */
  void OSMSCOUT_API ScanConvertLine(int x1, int y1,
                                    int x2, int y2,
                                    std::vector<ScanCell>& cells);

  /**
   * \ingroup Geometry
   * Return de distance of the point (px,py) to the segment [(p1x,p1y),(p2x,p2y)],
   * r the abscissa on the line of (qx,qy) the orthogonal projected point from (px,py).
   * 0 <= r <= 1 if q is between p1 and p2.
   */
  extern OSMSCOUT_API double distanceToSegment(double px, double py, double p1x, double p1y, double p2x, double p2y, double &r, double &qx, double &qy);

  class OSMSCOUT_API PolygonMerger
  {
  private:
    struct Edge
    {
      size_t fromIndex;
      size_t toIndex;
    };

  public:
    struct Polygon
    {
      std::vector<Point> coords;
    };

  private:
    std::unordered_map<Id,size_t>                               nodeIdIndexMap;
    std::vector<Point>                                          nodes;
    std::list<Edge>                                             edges;
    std::unordered_map<Id,std::list<std::list<Edge>::iterator>> idEdgeMap;

  private:
    void RemoveEliminatingEdges();

  public:
    void AddPolygon(const PointSequence& polygonsCoords);
    void AddPolygon(const std::vector<Point>& polygonsCoords);

    bool Merge(std::list<Polygon>& result);
  };

  struct OSMSCOUT_API CellDimension
  {
    double width;
    double height;
  };

  const size_t CELL_DIMENSION_MAX   = 25;
  const size_t CELL_DIMENSION_COUNT = CELL_DIMENSION_MAX+1;

  extern OSMSCOUT_API CellDimension cellDimension[CELL_DIMENSION_COUNT];
}

#endif
