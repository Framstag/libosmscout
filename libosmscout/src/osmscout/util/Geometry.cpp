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

#include <osmscout/private/Math.h>

#include <osmscout/util/Geometry.h>

#include <cassert>
#include <cstdlib>

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
    Calculating basic cost for the A* algorithm based on the
    spherical distance of two points on earth
    */
  double GetSphericalDistance(double aLon, double aLat,
                              double bLon, double bLat)
  {
    double r=6371.01; // Average radius of earth
    double dLat=(bLat-aLat)*M_PI/180;
    double dLon=(bLon-aLon)*M_PI/180;

    double a = sin(dLat/2)*sin(dLat/2)+cos(dLon/2)*cos(dLat/2)*sin(dLon/2)*sin(dLon/2);

    double c = 2*atan2(sqrt(a),sqrt(1-a));

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
    double sinU1=sin(U1);
    double cosU1=cos(U1);

    double U2=atan((1.0 - f) * tan(phi2));
    double sinU2=sin(U2);
    double cosU2=cos(U2);

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

      double sinlambda=sin(lambda);
      double coslambda=cos(lambda);

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
    double y=sin(dLon)*cos(bLat);
    double x=cos(aLat)*sin(bLat)-sin(aLat)*cos(bLat)*cos(dLon);

    double bearing=atan2(y,x);
    //double bearing=fmod(atan2(y,x)+2*M_PI,2*M_PI);

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
    double y=sin(dLon)*cos(aLat);
    double x=cos(bLat)*sin(aLat)-sin(bLat)*cos(aLat)*cos(dLon);

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

  void ScanConvertLine(const std::vector<Point>& points,
                       double xTrans, double cellWidth,
                       double yTrans, double cellHeight,
                       std::vector<ScanCell>& cells)
  {
    assert(points.size()>=2);

    for (size_t i=0; i<points.size()-1; i++) {
      int x1=int((points[i].GetLon()-xTrans)/cellWidth);
      int x2=int((points[i+1].GetLon()-xTrans)/cellWidth);
      int y1=int((points[i].GetLat()-yTrans)/cellHeight);
      int y2=int((points[i+1].GetLat()-yTrans)/cellHeight);

      ScanConvertLine(x1,y1,x2,y2,cells);
    }
  }
}
