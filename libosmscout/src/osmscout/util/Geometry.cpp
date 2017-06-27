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

#include <osmscout/util/Geometry.h>

#include <cstdlib>

#include <osmscout/system/Math.h>
#include <osmscout/system/SSEMathPublic.h>

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMath.h>
#endif

#include <osmscout/system/Assert.h>

namespace osmscout {

  size_t Pow(size_t a, size_t b)
  {
    if (b==0) {
      return 1;
    }

    size_t res=a;

    while (b>1) {
      res=res*a;
      b--;
    }

    return res;
  }

  /**
   * Calculates the distance between a point p and a line defined by the points a and b.
   * @param p
   *    The point in distance to a line
   * @param a
   *    One point defining the line
   * @param b
   *    Another point defining the line
   * @param intersection
   *    The point that is closest to 'p', either 'a', 'b', or a point on the line between
   *    'a' and 'b'.
   * @return
   *    The distance
   */
   double CalculateDistancePointToLineSegment(const GeoCoord& p,
                                              const GeoCoord& a,
                                              const GeoCoord& b,
                                              GeoCoord& intersection)
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

    intersection.Set(cy,cx);

    return sqrt(dx*dx+dy*dy);
  }

  /**
    Calculating basic cost for the A* algorithm based on the
    spherical distance of two points on earth
    */
  double GetSphericalDistance(const GeoCoord& a,
                              const GeoCoord& b)
  {
    double r=6371.01; // Average radius of earth
    double aLatRad=DegToRad(a.GetLat());
    double bLatRad=DegToRad(b.GetLat());
    double dLat=DegToRad(b.GetLat()-a.GetLat());
    double dLon=DegToRad(b.GetLon()-a.GetLon());

    double sindLonDiv2=sin(dLon/2);

    double aa = sin(dLat/2)*sin(dLat/2)+
        cos(aLatRad)*cos(bLatRad)*
        sindLonDiv2*sindLonDiv2;

    double c = 2*atan2(sqrt(aa),sqrt(1-aa));

    return r*c;
  }

  /**
    Calculating Vincenty's inverse for getting the ellipsoidal distance
    of two points on earth.
    */
  double GetEllipsoidalDistance(double aLon, double aLat,
                                double bLon, double bLat)
  {
    double a=6378137;
    double b=6356752.3142;
    double f=1/298.257223563;  // WGS-84 ellipsiod
    double phi1=aLat*M_PI/180;
    double phi2=bLat*M_PI/180;
    double lambda1=aLon*M_PI/180;
    double lambda2=bLon*M_PI/180;
    double a2b2b2=(a*a - b*b) / (b*b);

    double omega=lambda2 - lambda1;

    double U1=atan((1.0 - f) * tan(phi1));
    double sinU1;
    double cosU1;
    sincos(U1, sinU1, cosU1);

    double U2=atan((1.0 - f) * tan(phi2));
    double sinU2;
    double cosU2;
    sincos(U2, sinU2, cosU2);

    double sinU1sinU2=sinU1 * sinU2;
    double cosU1sinU2=cosU1 * sinU2;
    double sinU1cosU2=sinU1 * cosU2;
    double cosU1cosU2=cosU1 * cosU2;

    double lambda=omega;

    double A=0.0;
    double B=0.0;
    double sigma=0.0;
    double deltasigma=0.0;
    double lambda0;

    for (int i=0; i < 10; i++)
    {
      lambda0=lambda;

      double sinlambda;
      double coslambda;
      sincos(lambda, sinlambda, coslambda);

      double sin2sigma=(cosU2 * sinlambda * cosU2 * sinlambda) +
                        (cosU1sinU2 - sinU1cosU2 * coslambda) *
                        (cosU1sinU2 - sinU1cosU2 * coslambda);

      double sinsigma=sqrt(sin2sigma);

      double cossigma=sinU1sinU2 + (cosU1cosU2 * coslambda);

      sigma=atan2(sinsigma, cossigma);

      double sinalpha=(sin2sigma == 0) ? 0.0 :
                       cosU1cosU2 * sinlambda / sinsigma;

      double alpha=asin(sinalpha);
      double cosalpha=cos(alpha);
      double cos2alpha=cosalpha * cosalpha;

      double cos2sigmam=cos2alpha == 0.0 ? 0.0 :
                         cossigma - 2 * sinU1sinU2 / cos2alpha;

      double u2=cos2alpha * a2b2b2;

      double cos2sigmam2=cos2sigmam * cos2sigmam;

      A=1.0 + u2 / 16384 * (4096 + u2 *
                            (-768 + u2 * (320 - 175 * u2)));

      B=u2 / 1024 * (256 + u2 * (-128 + u2 * (74 - 47 * u2)));

      deltasigma=B * sinsigma * (cos2sigmam + B / 4 *
                                 (cossigma * (-1 + 2 * cos2sigmam2) - B / 6 *
                                  cos2sigmam * (-3 + 4 * sin2sigma) *
                                  (-3 + 4 * cos2sigmam2)));

      double C=f / 16 * cos2alpha * (4 + f * (4 - 3 * cos2alpha));

      lambda=omega + (1 - C) * f * sinalpha *
             (sigma + C * sinsigma * (cos2sigmam + C *
                                      cossigma * (-1 + 2 * cos2sigmam2)));

      if ((i > 1) && (std::abs((lambda - lambda0) / lambda) < 0.0000000000001)) {
        break;
      }
    }

    return b * A * (sigma - deltasigma)/1000; // We want the distance in Km
  }


  double GetEllipsoidalDistance(const GeoCoord& a,
                                const GeoCoord& b)
  {
    return GetEllipsoidalDistance(a.GetLon(),a.GetLat(),b.GetLon(),b.GetLat());
  }

  void GetEllipsoidalDistance(double lat1, double lon1,
                              double bearing, double distance,
                              double& lat2, double& lon2)
  {
    /* local variable definitions */

    // WGS-84 ellipsiod
    double a=6378137.0, b=6356752.3142, f=1/298.257223563;
    double alpha1,sinAlpha, sinAlpha1, cosAlpha1, cosSqAlpha;
    double sigma, sigma1, cos2SigmaM=0.0, sinSigma=0.0, cosSigma=0.0, deltaSigma=0.0, sigmaP=0.0;
    double tanU1, cosU1, sinU1, uSq;
    double A, B, C, L, lambda;
    double tmp;

    alpha1=bearing*M_PI/180;

    tanU1=(1-f)*tan(lat1*M_PI/180);

    cosAlpha1=cos(alpha1);
    sigma1=atan2(tanU1,cosAlpha1);

    cosU1=1/sqrt((1+tanU1*tanU1));
    sinAlpha1=sin(alpha1);
    sinAlpha=cosU1*sinAlpha1;

    cosSqAlpha=1-sinAlpha*sinAlpha;

    uSq=cosSqAlpha*(a*a-b*b)/(b*b);

    A=1+uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
    B=uSq/1024*(256+uSq*(-128+uSq*(74-47*uSq)));

    sigma=distance/(b*A);
    sigmaP=2*M_PI;
    while (fabs(sigma-sigmaP) > 1e-12) {
      cos2SigmaM = cos(2*sigma1 + sigma);
      sinSigma = sin(sigma);
      cosSigma = cos(sigma);
      deltaSigma = B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)-B/6*cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM)));
      sigmaP = sigma;
      sigma = distance / (b*A) + deltaSigma;
    }

    sinU1=tanU1*cosU1;

    tmp = sinU1*sinSigma - cosU1*cosSigma*cosAlpha1;
    lat2 = atan2(sinU1*cosSigma + cosU1*sinSigma*cosAlpha1,
        (1-f)*sqrt(sinAlpha*sinAlpha + tmp*tmp));
    lambda = atan2(sinSigma*sinAlpha1,
                   cosU1*cosSigma - sinU1*sinSigma*cosAlpha1);
    C = f/16*cosSqAlpha*(4+f*(4-3*cosSqAlpha));
    L = lambda - (1-C)*f*sinAlpha*(sigma+C*sinSigma*(cos2SigmaM+C*cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)));

    lat2=lat2*180.0/M_PI;
    lon2=lon1+L*180.0/M_PI;
  }

  /**
   * Taken the path from A to B over a sphere return the bearing (0..2PI) at the starting point A.
   */
  double GetSphericalBearingInitial(double aLon, double aLat,
                                    double bLon, double bLat)
  {
    aLon=aLon*M_PI/180;
    aLat=aLat*M_PI/180;

    bLon=bLon*M_PI/180;
    bLat=bLat*M_PI/180;

    double dLon=bLon-aLon;

    double sindLon, sinaLat, sinbLat;
    double cosdLon, cosaLat, cosbLat;
    sincos(dLon, sindLon, cosdLon);
    sincos(aLat, sinaLat, cosaLat);
    sincos(bLat, sinbLat, cosbLat);

    double y=sindLon*cosbLat;
    double x=cosaLat*sinbLat-sinaLat*cosbLat*cosdLon;

    double bearing=atan2(y,x);
    //double bearing=fmod(atan2(y,x)+2*M_PI,2*M_PI);

    return bearing;
  }

  double GetSphericalBearingInitial(const GeoCoord& a,
                                    const GeoCoord& b)
  {
    double aLon=a.GetLon()*M_PI/180;
    double aLat=a.GetLat()*M_PI/180;

    double bLon=b.GetLon()*M_PI/180;
    double bLat=b.GetLat()*M_PI/180;

    double dLon=bLon-aLon;

    double sindLon, sinaLat, sinbLat;
    double cosdLon, cosaLat, cosbLat;
    sincos(dLon, sindLon, cosdLon);
    sincos(aLat, sinaLat, cosaLat);
    sincos(bLat, sinbLat, cosbLat);

    double y=sindLon*cosbLat;
    double x=cosaLat*sinbLat-sinaLat*cosbLat*cosdLon;

    double bearing=atan2(y,x);

    return bearing;
  }

  /**
   * Taken the path from A to B over a sphere return the bearing (0..2PI) at the destination point B.
   */
  double GetSphericalBearingFinal(double aLon, double aLat,
                                  double bLon, double bLat)
  {
    aLon=aLon*M_PI/180;
    aLat=aLat*M_PI/180;

    bLon=bLon*M_PI/180;
    bLat=bLat*M_PI/180;

    double dLon=aLon-bLon;

    double sindLon, sinaLat, sinbLat;
    double cosdLon, cosaLat, cosbLat;
    sincos(dLon, sindLon, cosdLon);
    sincos(aLat, sinaLat, cosaLat);
    sincos(bLat, sinbLat, cosbLat);

    double y=sindLon*cosaLat;
    double x=cosbLat*sinaLat-sinbLat*cosaLat*cosdLon;

    double bearing=atan2(y,x);

    if (bearing>=0) {
      bearing-=M_PI;
    }
    else {
      bearing+=M_PI;
    }

    //double bearing=fmod(atan2(y,x)+3*M_PI,2*M_PI);

    return bearing;
  }

  /**
   * Taken the path from A to B over a sphere return the bearing (0..2PI) at the destination point B.
   */
  double GetSphericalBearingFinal(const GeoCoord& a,
                                  const GeoCoord& b)
  {
    double aLon=a.GetLon()*M_PI/180;
    double aLat=a.GetLat()*M_PI/180;

    double bLon=b.GetLon()*M_PI/180;
    double bLat=b.GetLat()*M_PI/180;

    double dLon=aLon-bLon;

    double sindLon, sinaLat, sinbLat;
    double cosdLon, cosaLat, cosbLat;
    sincos(dLon, sindLon, cosdLon);
    sincos(aLat, sinaLat, cosaLat);
    sincos(bLat, sinbLat, cosbLat);

    double y=sindLon*cosaLat;
    double x=cosbLat*sinaLat-sinbLat*cosaLat*cosdLon;

    double bearing=atan2(y,x);

    if (bearing>=0) {
      bearing-=M_PI;
    }
    else {
      bearing+=M_PI;
    }

    //double bearing=fmod(atan2(y,x)+3*M_PI,2*M_PI);

    return bearing;
  }

  std::string BearingDisplayString(double bearing)
  {
    int grad=(int)round(bearing*180/M_PI);

    grad=grad % 360;

    if (grad<0) {
      grad+=360;
    }

    if (grad>=0 && grad<=45) {
      return "N";
    }
    else if (grad>45 && grad<=135) {
      return "E";
    }
    else if (grad>135 && grad<=225) {
      return "S";
    }
    else if (grad>225 && grad<=315) {
      return "W";
    }
    else if (grad>315 && grad<360) {
      return "N";
    }

    return "?";
  }

  double NormalizeRelativeAngel(double angle)
  {
    if (angle>180.0) {
      return angle-360.0;
    }
    else if (angle<-180.0) {
      return angle+360.0;
    }

    return angle;
  }

  ScanCell::ScanCell(int x, int y)
  : x(x),
    y(y)
  {
    // no code
  }

  /**
   * This functions does a scan conversion of a line with the given start and end points.
   * This problem is equal to the following problem:
   * Assuming an index that works by referencing lines by linking them to all cells in a cell
   * grid that contain or are crossed by the line. Which cells does the line cross?
   *
   * The given vector for the result data is not cleared on start, to allow multiple calls
   * to this method with different line segments.
   *
   * The algorithm of Bresenham is used together with some checks for special cases.
   */
  void ScanConvertLine(int x1, int y1,
                       int x2, int y2,
                       std::vector<ScanCell>& cells)
  {
    bool steep=std::abs(y2-y1)>std::abs(x2-x1);

    if (steep) {
      std::swap(x1,y1);
      std::swap(x2,y2);
    }

    if (x1>x2) {
      std::swap(x1,x2);
      std::swap(y1,y2);
    }

    int dx=x2-x1;
    int dy=std::abs(y2-y1);
    int error=dx/2;
    int ystep;

    int y=y1;

    if (y1<y2) {
      ystep=1;
    }
    else {
      ystep=-1;
    }

    for (int x=x1; x<=x2; x++) {
      if (steep) {
        cells.push_back(ScanCell(y,x));
      }
      else {
        cells.push_back(ScanCell(x,y));
      }

      error-=dy;

      if (error<0) {
        y+=ystep;
        error+=dx;
      }
    }
  }


  /**
   * return the minimum distance from the point p to the line segment [p1,p2]
   * this could be the distance from p to p1 or to p2 if q the orthogonal projection of p
   * on the line supporting the segment is outside [p1,p2]
   * r is the abscissa of q on the line, 0 <= r <= 1 if q is between p1 and p2.
   */
  double DistanceToSegment(double px, double py,
                           double p1x, double p1y,
                           double p2x, double p2y,
                           double& r,
                           double& qx, double& qy)
  {
    if (p1x==p2x && p1y==p2y) {
        return NAN;
    }

    double rn=(px-p1x)*(p2x-p1x) + (py-p1y)*(p2y-p1y);
    double rd=(p2x-p1x)*(p2x-p1x) + (p2y-p1y)*(p2y-p1y);
    r=rn / rd;
    double ppx=p1x + r*(p2x-p1x);
    double ppy=p1y + r*(p2y-p1y);
    double s=((p1y-py)*(p2x-p1x)-(p1x-px)*(p2y-p1y)) / rd;

    if ((r >= 0) && (r <= 1)) {
      qx=ppx;
      qy=ppy;

      return fabs(s)*sqrt(rd);
    }
    else {
      double dist1=(px-p1x)*(px-p1x) + (py-p1y)*(py-p1y);
      double dist2=(px-p2x)*(px-p2x) + (py-p2y)*(py-p2y);

      if (dist1<dist2) {
        qx=p1x;
        qy=p1y;

        return sqrt(dist1);
      }
      else {
        qx=p2x;
        qy=p2y;

        return sqrt(dist2);
      }
    }
  }

  void PolygonMerger::AddPolygon(const std::vector<Point>& polygonCoords)
  {
    assert(polygonCoords.size()>=3);

    std::vector<Point> coords(polygonCoords);

    if (!AreaIsClockwise(polygonCoords)) {
      std::reverse(coords.begin(),coords.end());
    }

    nodes.reserve(nodes.size()+coords.size());

    // Optimize: index retrieval (next is current in the next iteration)
    // Optimize: No need to reverse the arrays, if we just traverse the other way round as second case

    for (size_t i=0; i<coords.size(); i++) {
      size_t current=i;
      size_t next=(i+1==coords.size()) ? 0 : i+1;

      auto currentNode=nodeIdIndexMap.find(coords[current].GetId());
      auto nextNode=nodeIdIndexMap.find(coords[next].GetId());

      if (currentNode==nodeIdIndexMap.end()) {
        Point node=coords[current];

        nodes.push_back(node);

        currentNode=nodeIdIndexMap.insert(std::make_pair(node.GetId(),nodes.size()-1)).first;
      }

      if (nextNode==nodeIdIndexMap.end()) {
        Point node=coords[next];

        nodes.push_back(node);

        nextNode=nodeIdIndexMap.insert(std::make_pair(node.GetId(),nodes.size()-1)).first;
      }

      Edge edge;

      edge.fromIndex=currentNode->second;
      edge.toIndex=nextNode->second;

      edges.push_back(edge);
    }
  }

  void PolygonMerger::RemoveEliminatingEdges()
  {
    std::unordered_map<Id,std::list<std::list<Edge>::iterator > > idEdgeMap;

    // We first group edge by the sum of the from and to index. This way,
    // were can be sure, that a node(a=>b) is in the same list as a node(b=>a).
    for (std::list<Edge>::iterator edge=edges.begin();
        edge!=edges.end();
        ++edge) {
      idEdgeMap[edge->fromIndex+edge->toIndex].push_back(edge);
    }

    for (auto& edgeList : idEdgeMap) {
      auto currentEdge=edgeList.second.begin();

      while (currentEdge!=edgeList.second.end()) {
        auto lookup=currentEdge;

        lookup++;
        while (lookup!=edgeList.second.end() &&
            !((*currentEdge)->fromIndex==(*lookup)->toIndex &&
              (*currentEdge)->toIndex==(*lookup)->fromIndex)) {
          lookup++;
        }

        if (lookup!=edgeList.second.end()) {
          //std::cout << "Delete matching edges " << (*currentEdge)->fromIndex << "=>" << (*currentEdge)->toIndex << " and " << (*lookup)->fromIndex << "=>" << (*lookup)->toIndex << std::endl;
          // Delete lookup from list of edges
          edges.erase(*lookup);
          // Delete currentEdge from list of edges
          edges.erase(*currentEdge);
          // Delete lookup from list
          edgeList.second.erase(lookup);
          // Delete currentEdge from list
          currentEdge=edgeList.second.erase(currentEdge);
        }
        else {
          currentEdge++;
        }
      }
    }
  }

  bool PolygonMerger::Merge(std::list<Polygon>& result)
  {
    result.clear();

    RemoveEliminatingEdges();

    std::unordered_map<Id,std::list<std::list<Edge>::iterator > > idEdgeMap;

    // We first group edge by from index.
    for (std::list<Edge>::iterator edge=edges.begin();
        edge!=edges.end();
        ++edge) {
      //std::cout << "* " << edge->fromIndex << "=>" << edge->toIndex << std::endl;
      idEdgeMap[edge->fromIndex].push_back(edge);
    }

    std::list<std::list<Edge> > polygons;

    while (!edges.empty()) {
      std::list<Edge>::iterator current=edges.begin();

      // Start a new polygon
      //std::cout << "---" << std::endl;
      polygons.push_back(std::list<Edge>());

      //std::cout << "* " << current->fromIndex << "=>" << current->toIndex << std::endl;
      polygons.back().push_back(*current);

      // Delete entry from the idEdgeMap
      std::unordered_map<Id,std::list<std::list<Edge>::iterator > >::iterator mapEntry;

      mapEntry=idEdgeMap.find(current->fromIndex);

      assert(mapEntry!=idEdgeMap.end());

      for (std::list<std::list<Edge>::iterator>::iterator edge=mapEntry->second.begin();
          edge!=mapEntry->second.end();
          ++edge) {
        if (*edge==current) {
          mapEntry->second.erase(edge);
          break;
        }
      }

      // Delete entry from the list of edges
      edges.pop_front();

      do {
        mapEntry=idEdgeMap.find(polygons.back().back().toIndex);

        if (mapEntry->second.empty()) {
          //std::cerr << "No matching node found" << std::endl;
          return false;
        }

        current=mapEntry->second.front();

        mapEntry->second.pop_front();

        //std::cout << "* " << current->fromIndex << "=>" << current->toIndex << std::endl;
        polygons.back().push_back(*current);

        edges.erase(current);
      } while (!edges.empty() && polygons.back().back().toIndex!=polygons.back().front().fromIndex);

      if (polygons.back().back().toIndex!=polygons.back().front().fromIndex) {
        //std::cerr << "Merges run out of edges" << std::endl;
        return false;
      }
    }

    for (const auto& polygon : polygons) {
      result.push_back(Polygon());

      for (const auto& edge : polygon) {
        result.back().coords.push_back(nodes[edge.fromIndex]);
      }
    }

    return true;
  }

  CellDimension cellDimension[] = {
      { 360.0,                      180.0                      }, //  0
      { 180.0,                       90.0                      }, //  1
      {  90.0,                       45.0                      }, //  2
      {  45.0,                       22.5                      }, //  3
      {  22.5,                       11.25                     }, //  4
      {  11.25,                       5.625                    }, //  5
      {   5.625,                      2.8125                   }, //  6
      {   2.8125,                     1.40625                  }, //  7
      {   1.40625,                    0.703125                 }, //  8
      {   0.703125,                   0.3515625                }, //  9
      {   0.3515625,                  0.17578125               }, // 10
      {   0.17578125,                 0.087890625              }, // 11
      {   0.087890625,                0.0439453125             }, // 12
      {   0.0439453125,               0.02197265625            }, // 13
      {   0.02197265625,              0.010986328125           }, // 14
      {   0.010986328125,             0.0054931640625          }, // 15
      {   0.0054931640625,            0.00274658203125         }, // 16
      {   0.00274658203125,           0.001373291015625        }, // 17
      {   0.001373291015625,          0.0006866455078125       }, // 18
      {   0.0006866455078125,         0.00034332275390625      }, // 19
      {   0.00034332275390625,        0.000171661376953125     }, // 20
      {   0.000171661376953125,       0.0000858306884765625    }, // 21
      {   0.0000858306884765625,      0.00004291534423828125   }, // 22
      {   0.00004291534423828125,     0.000021457672119140625  }, // 23
      {   0.000021457672119140625,    0.0000107288360595703125 }, // 24
      {   0.0000107288360595703125,   0.0000107288360595703125 }  // 25
  };
}
