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
#include <functional>
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
#include <osmscout/Types.h>

#include <osmscout/util/GeoBox.h>

namespace osmscout {

  inline double DegToRad(double deg)
  {
    return  deg * M_PI / 180;
  }

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
    if (a1.IsEqual(a2) &&
        b1.IsEqual(b2)){
      // two different zero size vectors can't intersects
      return false;
    }

    // denominator, see GetLineIntersection for more details
    double denr=(b2.GetLat()-b1.GetLat())*(a2.GetLon()-a1.GetLon())-
                (b2.GetLon()-b1.GetLon())*(a2.GetLat()-a1.GetLat());

    double ua_numr=(b2.GetLon()-b1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (b2.GetLat()-b1.GetLat())*(a1.GetLon()-b1.GetLon());
    double ub_numr=(a2.GetLon()-a1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (a2.GetLat()-a1.GetLat())*(a1.GetLon()-b1.GetLon());

    if (denr==0.0) { // parallels
      if (ua_numr==0.0 && ub_numr==0.0) {
        // two lines are on one straight line, check the bounds
        GeoBox aBox(GeoCoord(a1.GetLat(),a1.GetLon()),
                    GeoCoord(a2.GetLat(),a2.GetLon()));
        GeoBox bBox(GeoCoord(b1.GetLat(),b1.GetLon()),
                    GeoCoord(b2.GetLat(),b2.GetLon()));

        return bBox.Includes(a1,false) ||
               bBox.Includes(a2,false) ||
               aBox.Includes(b1,false) ||
               aBox.Includes(b2,false);
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
  template<typename N,typename I>
  bool GetLineIntersection(const N& a1,
                           const N& a2,
                           const N& b1,
                           const N& b2,
                           I& intersection)
  {
    if (a1.IsEqual(b1) ||
        a1.IsEqual(b2)){
      intersection.Set(a1.GetLat(),a1.GetLon());
      return true;
    }
    if (a2.IsEqual(b1) ||
        a2.IsEqual(b2)) {
      intersection.Set(a2.GetLat(),a2.GetLon());
      return true;
    }
    if (a1.IsEqual(a2) &&
        b1.IsEqual(b2)){
      // two different zero size vectors can't intersects
      return false;
    }

    // for geographic, expect point.x=lon and point.y=lat

    // see https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
    // and http://www.geeksforgeeks.org/orientation-3-ordered-points/

    // denominator < 0  : left angle
    // denominator == 0 : parallels
    // denominator > 0  : right angle
    double denr=(b2.GetLat()-b1.GetLat())*(a2.GetLon()-a1.GetLon())-
                (b2.GetLon()-b1.GetLon())*(a2.GetLat()-a1.GetLat());

    double ua_numr=(b2.GetLon()-b1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (b2.GetLat()-b1.GetLat())*(a1.GetLon()-b1.GetLon());
    double ub_numr=(a2.GetLon()-a1.GetLon())*(a1.GetLat()-b1.GetLat())-
                   (a2.GetLat()-a1.GetLat())*(a1.GetLon()-b1.GetLon());

    if (denr==0.0) {
      if (ua_numr==0.0 && ub_numr==0.0) {
        // two lines are on one straight line, check the bounds
        GeoBox aBox(GeoCoord(a1.GetLat(),a1.GetLon()),
                    GeoCoord(a2.GetLat(),a2.GetLon()));
        GeoBox bBox(GeoCoord(b1.GetLat(),b1.GetLon()),
                    GeoCoord(b2.GetLat(),b2.GetLon()));
        if (bBox.Includes(a1,false)){
          intersection.Set(a1.GetLat(),a1.GetLon());
          return true;
        }
        if (bBox.Includes(a2,false)){
          intersection.Set(a2.GetLat(),a2.GetLon());
          return true;
        }
        if (aBox.Includes(b1,false)){
          intersection.Set(b1.GetLat(),b1.GetLon());
          return true;
        }
        if (aBox.Includes(b2,false)){
          intersection.Set(b2.GetLat(),b2.GetLon());
          return true;
        }

        return false;
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

  /**
   * \ingroup Geometry
   *
   * Returns true, if the lines defined by the given coordinates intersect. Returns the intersection.
   */
  template<typename N>
  bool GetLineIntersectionPixel(const N& a1,
                                const N& a2,
                                const N& b1,
                                const N& b2,
                                N& intersection)
  {
    if (a1.IsEqual(b1) ||
        a1.IsEqual(b2)){
      intersection.Set(a1.GetX(),a1.GetY());
      return true;
    }
    if (a2.IsEqual(b1) ||
        a2.IsEqual(b2)) {
      intersection.Set(a2.GetX(),a2.GetY());
      return true;
    }
    if (a1.IsEqual(a2) &&
        b1.IsEqual(b2)){
      // two different zero size vectors can't intersects
      return false;
    }

    double denr=(b2.GetY()-b1.GetY())*(a2.GetX()-a1.GetX())-
                (b2.GetX()-b1.GetX())*(a2.GetY()-a1.GetY());

    double ua_numr=(b2.GetX()-b1.GetX())*(a1.GetY()-b1.GetY())-
                   (b2.GetY()-b1.GetY())*(a1.GetX()-b1.GetX());
    double ub_numr=(a2.GetX()-a1.GetX())*(a1.GetY()-b1.GetY())-
                   (a2.GetY()-a1.GetY())*(a1.GetX()-b1.GetX());

    if (denr==0.0) {
      if (ua_numr==0.0 && ub_numr==0.0) {
        // This gives currently false hits because of number resolution problems, if two lines are very
        // close together and for example are part of a very details node curve intersections are detected.

        // FIXME: setup intersection
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
      intersection.Set(a1.GetX()+ua*(a2.GetX()-a1.GetX()),
                       a1.GetY()+ua*(a2.GetY()-a1.GetY()));
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
   *
   * Returns true, if point in on the area border or within the area.
   *
   * See http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
   */
  template<typename N, typename M>
  inline bool IsCoordInArea(const N& point,
                            const std::vector<M>& nodes)
  {
    size_t i,j;
    bool   c=false;

    for (i=0, j=nodes.size()-1; i<nodes.size(); j=i++) {
      if (point.GetLat()==nodes[i].GetLat() &&
          point.GetLon()==nodes[i].GetLon()) {
        return true;
      }

      if ((((nodes[i].GetLat()<=point.GetLat()) && (point.GetLat()<nodes[j].GetLat())) ||
           ((nodes[j].GetLat()<=point.GetLat()) && (point.GetLat()<nodes[i].GetLat()))) &&
          (point.GetLon()<(nodes[j].GetLon()-nodes[i].GetLon())*(point.GetLat()-nodes[i].GetLat())/(nodes[j].GetLat()-nodes[i].GetLat())+
           nodes[i].GetLon())) {
        c=!c;
      }
    }

    return c;
  }

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
   * Return true, if at least one point of area a in within area b
   */
  template<typename N,typename M>
  inline bool IsAreaAtLeastPartlyInArea(const std::vector<N>& a,
                                        const std::vector<M>& b,
                                        const GeoBox aBox,
                                        const GeoBox bBox)
   {
    if (!aBox.Intersects(bBox)){
      return false;
    }

    for (const auto& node : a) {
      if (bBox.Includes(node, /*openInterval*/ false) && GetRelationOfPointToArea(node,b)>=0) {
        return true;
      }
    }

    return false;
  }

  /**
   * \ingroup Geometry
   * Return true, if at least one point of area a in within area b
   */
  template<typename N,typename M>
  inline bool IsAreaAtLeastPartlyInArea(const std::vector<N>& a,
                                        const std::vector<M>& b)
  {
    GeoBox aBox;
    GeoBox bBox;
    GetBoundingBox(a, aBox);
    GetBoundingBox(b, bBox);

    return IsAreaAtLeastPartlyInArea(a,b,aBox,bBox);
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

      if (count>=100 && pro/20.0>contra) {
        return true;
      }
    }

    return pro/20.0>contra;
  }

  /**
   * \ingroup Geometry
   * Assumes that the given areas do not intersect.
   *
   * Returns true, of area a is within b or the same as b. This
   * version uses some heuristic based on the assumption that areas
   * are either in another area or not - but there may be some smaller
   * errors due to areas slightly overlapping.
   */
  template<typename N,typename M>
  inline bool IsAreaSubOfAreaOrSame(const std::vector<N>& a,
                                    const std::vector<M>& b)
  {
    size_t pro=0;
    size_t contra=0;
    size_t count=0;

    pro=0;
    contra=0;
    count=0;

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

    if (pro == 0 && contra == 0 && count > 0)
      return true;

    return pro/20.0>contra;
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

    for (size_t i=1; i<edges.size(); i++) {
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
    int nextIdx=(ptIdx==(int)edges.size()-1) ? 0 : ptIdx+1;

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
   *Calculates the initial bearing for a line from one coordinate to the other coordinate
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
   * \ingroup Geometry
   *Calculates the final bearing for a line from one coordinate two the other coordinate
   *on a sphere.
   */
  extern OSMSCOUT_API double GetSphericalBearingFinal(const GeoCoord& a,
                                                      const GeoCoord& b);

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

    inline void Set(int x, int y)
    {
      this->x=x;
      this->y=y;
    }

    inline bool operator==(const ScanCell& other) const
    {
      return x==other.x && y==other.y;
    }

    inline bool IsEqual(const ScanCell& other) const
    {
      return x==other.x && y==other.y;
    }

    inline int GetX() const
    {
      return x;
    }

    inline int GetY() const
    {
      return y;
    }
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
  extern OSMSCOUT_API double DistanceToSegment(double px, double py,
                                               double p1x, double p1y,
                                               double p2x, double p2y,
                                               double& r,
                                               double& qx, double& qy);

  /**
   * \ingroup Geometry
   * Information about intersection of two paths
   */
  struct OSMSCOUT_API PathIntersection
  {
    GeoCoord point;           //!< intersection point
    size_t   aIndex;          //!< "a path" point index before intersection
    size_t   bIndex;          //!< "b path" point index before intersection
    double   orientation;     //!< angle between a -> intersection -> b
                              //!<   orientation > 0 = left angle
    double   aDistanceSquare; //!< distance^2 between "a path" point and intersection
    double   bDistanceSquare; //!< distance^2 between "b path" point and intersection
  };

  /**
   * Helper for FindPathIntersections
   *
   * Compute bounding box of path segment from node index
   * `from` (inclusive) to node index `to` (exclusive)
   */
  template<typename N>
  void GetSegmentBoundingBox(const std::vector<N>& path,
                             size_t from, size_t to,
                             GeoBox& boundingBox)
  {
    if (path.empty() || from>=to){
      boundingBox.Invalidate();
    }

    double minLon=path[from%path.size()].GetLon();
    double maxLon=path[from%path.size()].GetLon();
    double minLat=path[from%path.size()].GetLat();
    double maxLat=path[from%path.size()].GetLat();

    for (size_t i=from; i<to; i++) {
      minLon=std::min(minLon,path[i%path.size()].GetLon());
      maxLon=std::max(maxLon,path[i%path.size()].GetLon());
      minLat=std::min(minLat,path[i%path.size()].GetLat());
      maxLat=std::max(maxLat,path[i%path.size()].GetLat());
    }

    boundingBox.Set(GeoCoord(minLat,minLon),
                    GeoCoord(maxLat,maxLon));
  }

  /**
   * Helper for FindPathIntersections
   *
   * Compute bounding boxes for path segments with length 1000 nodes,
   * up to node index bound (exclusive)
   */
  template<typename N>
  void ComputeSegmentBoxes(const std::vector<N>& path,
                           std::vector<GeoBox> &segmentBoxes,
                           size_t bound)
  {
    for (size_t i=0;i<bound;i+=1000){
      GeoBox box;
      GetSegmentBoundingBox(path,i,std::min(i+1000,bound), box);
      segmentBoxes.push_back(box);
    }
  }

  /**
   * \ingroup Geometry
   * Find all intersections between aPath and bPath from node index aStartIndex
   * and bStartIndex.
   * Intersections are added to intersections vector.
   */
  template<typename N>
  void FindPathIntersections(const std::vector<N> &aPath,
                             const std::vector<N> &bPath,
                             bool aClosed,
                             bool bClosed,
                             std::vector<PathIntersection> &intersections,
                             size_t aStartIndex=0,
                             size_t bStartIndex=0)
  {
    size_t aIndex=aStartIndex;
    size_t bIndex=bStartIndex;
    // bounds are inclusive
    size_t aBound=aClosed?aIndex+aPath.size():aPath.size()-1;
    size_t bBound=bClosed?bIndex+bPath.size():bPath.size()-1;
    size_t bStart=bIndex;

    if (bStart>=bBound || aIndex>=aBound){
      return;
    }

    GeoBox bBox;
    GetSegmentBoundingBox(bPath,bIndex,bBound+1,bBox);
    GeoBox aLineBox;
    GeoBox bLineBox;

    // compute b-boxes for B path, each 1000 point-long segment
    std::vector<GeoBox> bSegmentBoxes;
    ComputeSegmentBoxes(bPath,bSegmentBoxes,bBound+1);

    for (;aIndex<aBound;aIndex++){
      N a1=aPath[aIndex%aPath.size()];
      N a2=aPath[(aIndex+1)%aPath.size()];
      aLineBox.Set(GeoCoord(a1.GetLat(),a1.GetLon()),
                   GeoCoord(a2.GetLat(),a2.GetLon()));
      if (!bBox.Intersects(aLineBox,/*openInterval*/false)){
        continue;
      }
      for (bIndex=bStart;bIndex<bBound;bIndex++){
        N b1=bPath[bIndex%bPath.size()];
        N b2=bPath[(bIndex+1)%bPath.size()];
        bLineBox.Set(GeoCoord(b1.GetLat(),b1.GetLon()),
                     GeoCoord(b2.GetLat(),b2.GetLon()));

        if (!bSegmentBoxes[bIndex/1000].Intersects(aLineBox,/*openInterval*/false) &&
            !aLineBox.Intersects(bLineBox,/*openInterval*/false)){
          // round up
          bIndex+=std::max(0, 998-(int)(bIndex%1000));
          continue;
        }
        if (!aLineBox.Intersects(bLineBox,/*openInterval*/false)){
          continue;
        }
        PathIntersection pi;
        if (GetLineIntersection(a1,a2,
                                b1,b2,
                                pi.point)){

          // if b2==pi or a1==pi, orientation is zero then
          // for that reason, we prolong both lines for computation
          // of intersection orientation
          GeoCoord pointBefore(a1.GetLat()-(a2.GetLat()-a1.GetLat()),
                               a1.GetLon()-(a2.GetLon()-a1.GetLon()));
          GeoCoord pointAfter(b2.GetLat()+(b2.GetLat()-b1.GetLat()),
                              b2.GetLon()+(b2.GetLon()-b1.GetLon()));
          pi.orientation=(pi.point.GetLon()-pointBefore.GetLon())*
                         (pointAfter.GetLat()-pi.point.GetLat())-
                         (pi.point.GetLat()-pointBefore.GetLat())*
                         (pointAfter.GetLon()-pi.point.GetLon());

          pi.aIndex=aIndex;
          pi.bIndex=bIndex;
          pi.aDistanceSquare=DistanceSquare(GeoCoord(a1.GetLat(),a1.GetLon()),pi.point);
          pi.bDistanceSquare=DistanceSquare(GeoCoord(b1.GetLat(),b1.GetLon()),pi.point);

          intersections.push_back(pi);
        }
      }
    }
  }

  /**
   * \ingroup Geometry
   * Find next intersetion on way (with itself) from node index i.
   * Return true if some intersection was found (way is not simple),
   * i and j indexes are setup to start possition of intesections lines.
   */
  template<typename N>
  bool FindIntersection(const std::vector<N> &way,
                        size_t &i,
                        size_t &j)
  {
    GeoBox lineBox;

    // compute b-boxes for path, each 1000 point-long segment
    std::vector<GeoBox> segmentBoxes;
    ComputeSegmentBoxes(way,segmentBoxes,way.size());

    for (; i<way.size()-1; i++) {
      N i1=way[i];
      N i2=way[i+1];
      lineBox.Set(GeoCoord(i1.GetLat(),i1.GetLon()),
                  GeoCoord(i2.GetLat(),i2.GetLon()));

      for (j=i+1; j<way.size()-1; j++) {
        if (!segmentBoxes[j/1000].Intersects(lineBox,/*openInterval*/false) &&
            !segmentBoxes[(j+1)/1000].Intersects(lineBox,/*openInterval*/false)){
          // round up
          j+=std::max(0, 998-(int)(j%1000));
          continue;
        }
        if (LinesIntersect(way[i],
                           way[i+1],
                           way[j],
                           way[j+1])) {

          if (way[i].IsEqual(way[j]) ||
              way[i].IsEqual(way[j+1]) ||
              way[i+1].IsEqual(way[j]) ||
              way[i+1].IsEqual(way[j+1])) {
            continue; // ignore sibling edges
          }

          return true;
        }
      }
    }
    return false;
  }

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

namespace std {
  template <>
  struct hash<osmscout::ScanCell>
  {
    size_t operator()(const osmscout::ScanCell& cell) const
    {
      return hash<int>{}(cell.GetX()) ^ (hash<int>{}(cell.GetY()) << 1);
    }
  };
}

#endif
