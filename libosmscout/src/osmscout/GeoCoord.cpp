/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/GeoCoord.h>

#include <sstream>

namespace osmscout {

  const double latConversionFactor=134217727.0/180.0; // 27 Bit
  const double lonConversionFactor=134217727.0/360.0; // 27 Bit

  Id GeoCoord::GetId() const
  {
    Id id;

    uint64_t latValue=(uint64_t)round((lat+90.0)*latConversionFactor);
    uint64_t lonValue=(uint64_t)round((lon+180.0)*lonConversionFactor);

    id=((latValue & 0x000000ff) <<  8)+  // 0 => 8
       ((lonValue & 0x000000ff) <<  0)+  // 0 => 0
       ((latValue & 0x0000ff00) << 16)+  // 8 => 24
       ((lonValue & 0x0000ff00) <<  8)+  // 8 => 16
       ((latValue & 0x00ff0000) << 24)+  // 16 => 40
       ((lonValue & 0x00ff0000) << 16)+  // 16 => 32
       ((latValue & 0x07000000) << 27)+  // 24 => 51
       ((lonValue & 0x07000000) << 24);  // 24 => 48

    return id;
  }

  std::string GeoCoord::GetDisplayText() const
  {
    std::ostringstream      stream;
    std::streamsize         oldPrecision=stream.precision(5);
    std::ios_base::fmtflags oldFlags=stream.setf(std::ios::fixed,std::ios::floatfield);

    stream.imbue(std::locale());

    stream << std::abs(GetLat());

    if (GetLat()>=0) {
      stream << " N ";
    }
    else {
      stream << " S ";
    }

    stream << std::abs(GetLon());

    if (GetLon()>=0) {
      stream << " E";
    }
    else {
      stream << " W";
    }

    stream.precision(oldPrecision);
    stream.setf(oldFlags,std::ios::floatfield);

    return stream.str();
  }

  static size_t EatWhitespace(const std::string& text,
                              size_t currentPos)
  {
    while (currentPos<text.length() &&
        text[currentPos]==' ') {
      currentPos++;
    }

    return currentPos;
  }

  static bool ScanCoordinate(const std::string& text,
                             size_t& currentPos,
                             double& value,
                             size_t maxDigits)
  {
    size_t digits=0;

    value=0.0;

    while (currentPos<text.length() &&
        text[currentPos]>='0' &&
        text[currentPos]<='9') {
      digits++;

      if (value==0.0) {
        value=text[currentPos]-'0';
      }
      else {
        value=value*10+(text[currentPos]-'0');
      }

      if (digits>maxDigits) {
        return false;
      }

      currentPos++;
    }

    if (digits==0) {
      return false;
    }

    // TODO: Scan Digit and Co

    if (currentPos<text.length() &&
        (text[currentPos]=='.' ||
         text[currentPos]==',')) {

      currentPos++;

      double factor=10;

      while (currentPos<text.length() &&
            text[currentPos]>='0' &&
            text[currentPos]<='9') {
        value=value+(text[currentPos]-'0')/factor;

        factor=factor*10;
        currentPos++;
      }
    }

    return true;
  }

  bool GeoCoord::Parse(const std::string& text,
                       GeoCoord& coord)
  {
    size_t currentPos=0;

    double latitude=0;
    bool   latPos=true;
    bool   latDirectionGiven=false;

    double longitude=0;
    bool   lonPos=true;
    bool   lonDirectionGiven=false;

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      return false;
    }

    //
    // Latitude
    //

    if (text[currentPos]=='N') {
      latPos=true;
      latDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='S') {
      latPos=false;
      latDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='+') {
      latPos=true;
      latDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='-') {
      latPos=false;
      latDirectionGiven=true;
      currentPos++;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      return false;
    }

    if (!ScanCoordinate(text,
                        currentPos,
                        latitude,
                        2)) {
      return false;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      return false;
    }

    if (text[currentPos]=='N') {
      if (latDirectionGiven) {
        return false;
      }

      latPos=true;
      latDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='S') {
      if (latDirectionGiven) {
        return false;
      }

      latPos=false;
      latDirectionGiven=true;
      currentPos++;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      return false;
    }

    if (!latPos) {
      latitude=-latitude;
    }

    //
    // Longitude
    //

    if (text[currentPos]=='E') {
      lonPos=true;
      lonDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='W') {
      lonPos=false;
      lonDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='+') {
      lonPos=true;
      lonDirectionGiven=true;
      currentPos++;
    }
    else if (text[currentPos]=='-') {
      lonPos=false;
      lonDirectionGiven=true;
      currentPos++;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      return false;
    }

    if (!ScanCoordinate(text,
                        currentPos,
                        longitude,
                        3)) {
      return false;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos<text.length()) {
      if (text[currentPos]=='E') {
        if (lonDirectionGiven) {
          return false;
        }

        lonPos=true;
        lonDirectionGiven=true;
        currentPos++;
      }
      else if (text[currentPos]=='W') {
        if (lonDirectionGiven) {
          return false;
        }

        lonPos=false;
        lonDirectionGiven=true;
        currentPos++;
      }
    }

    if (!lonPos) {
      longitude=-longitude;
    }

    currentPos=EatWhitespace(text,
                             currentPos);

    if (currentPos>=text.length()) {
      coord.Set(latitude,
                longitude);

      return true;
    }
    else {
      return false;
    }
  }

  double GeoCoord::GetDistance(GeoCoord target)
  {
      double a = 6378137.0;
      double b = 6.3568e+06;
      double f = 0.0034;
      double a2b2b2 = (a * a - b * b) / (b * b);
      double omega = (target.lon - lon) * M_PI / 180.0;
      double U1 = atan((1.0 - f) * tan(lat * M_PI / 180.0));
      double sinU1 = sin(U1);
      double cosU1 = cos(U1);
      double U2 = atan((1.0 - f) * tan(target.lat * M_PI / 180.0));
      double sinU2 = sin(U2);
      double cosU2 = cos(U2);
      double sinU1sinU2 = sinU1 * sinU2;
      double cosU1sinU2 = cosU1 * sinU2;
      double sinU1cosU2 = sinU1 * cosU2;
      double cosU1cosU2 = cosU1 * cosU2;
      double lambda = omega;
      double A = 0.0;
      double B = 0.0;
      double sigma = 0.0;
      double deltasigma = 0.0;
      double lambda0;
      for (int i = 0; i < 20; i++)
      {
          lambda0 = lambda;
          double sinlambda = sin(lambda);
          double coslambda = cos(lambda);
          double sin2sigma = (cosU2 * sinlambda * cosU2 * sinlambda) + pow(cosU1sinU2 - sinU1cosU2 * coslambda, 2.0);
          double sinsigma = sqrt(sin2sigma);
          double cossigma = sinU1sinU2 + (cosU1cosU2 * coslambda);
          sigma = atan2(sinsigma, cossigma);
          double sinalpha = (sin2sigma == 0) ? 0.0 : cosU1cosU2 * sinlambda / sinsigma;
          double alpha = asin(sinalpha);
          double cosalpha = cos(alpha);
          double cos2alpha = cosalpha * cosalpha;
          double cos2sigmam = cos2alpha == 0.0 ? 0.0 : cossigma - 2 * sinU1sinU2 / cos2alpha;
          double u2 = cos2alpha * a2b2b2;
          double cos2sigmam2 = cos2sigmam * cos2sigmam;
          A = 1.0 + u2 / 16384.0 * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
          B = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
          deltasigma = B * sinsigma * (cos2sigmam + B / 4 * (cossigma * (-1.0 + 2.0 * cos2sigmam2) - B / 6.0 * cos2sigmam * (-3.0 + 4.0 * sin2sigma) * (-3.0 + 4.0 * cos2sigmam2)));
          double C = f / 16.0 * cos2alpha * (4.0 + f * (4.0 - 3.0 * cos2alpha));
          lambda = omega + (1.0 - C) * f * sinalpha * (sigma + C * sinsigma * (cos2sigmam + C * cossigma * (-1.0 + 2.0 * cos2sigmam2)));
          if ((i > 1) && (abs((lambda - lambda0) / lambda) < 0.0000000000001)) break;
      }
      return b * A * (sigma - deltasigma);
  }

  GeoCoord GeoCoord::Add(double bearing, double distance)
  {
      if (distance == 0.0) return *this;
      double a = 6378137;
      double b = 6.3568e+06;
      double f = 0.0034;
      double alpha1 = bearing * M_PI / 180.0;
      double cosAlpha1 = cos(alpha1);
      double sinAlpha1 = sin(alpha1);
      double s = distance;
      double tanU1 = (1.0 - f) * tan(lat * M_PI / 180.0);
      double cosU1 = 1.0 / sqrt(1.0 + tanU1 * tanU1);
      double sinU1 = tanU1 * cosU1;
      double sigma1 = atan2(tanU1, cosAlpha1);
      double sinAlpha = cosU1 * sinAlpha1;
      double sin2Alpha = sinAlpha * sinAlpha;
      double cos2Alpha = 1.0 - sin2Alpha;
      double uSquared = cos2Alpha * (a * a - b * b) / (b * b);
      double A = 1.0 + (uSquared / 16384.0) * (4096.0 + uSquared * (-768.0 + uSquared * (320.0 - 175.0 * uSquared)));
      double B = (uSquared / 1024.0) * (256.0 + uSquared * (-128.0 + uSquared * (74.0 - 47.0 * uSquared)));
      double sOverbA = s / (b * A);
      double sigma = sOverbA;
      double sinSigma;
      double prevSigma = sOverbA;
      double cosSigmaM2;
      for (;;)
      {
          cosSigmaM2 = cos(2.0 * sigma1 + sigma);
          sinSigma = sin(sigma);
          double cosSignma = cos(sigma);
          sigma = sOverbA + (B * sinSigma * (cosSigmaM2 + (B / 4.0) * (cosSignma * (-1.0 + 2.0 * cosSigmaM2 * cosSigmaM2) - (B / 6.0) * cosSigmaM2 * (-3.0 + 4.0 * sinSigma * sinSigma) * (-3.0 + 4.0 * cosSigmaM2 * cosSigmaM2))));
          if (abs(sigma - prevSigma) < 0.0000000000001) break;
          prevSigma = sigma;
      }
      cosSigmaM2 = cos(2.0 * sigma1 + sigma);
      double cosSigma = cos(sigma);
      sinSigma = sin(sigma);
      double phi2 = atan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1, (1.0 - f) * sqrt(sin2Alpha + pow(sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1, 2.0)));
      double lambda = atan2(sinSigma * sinAlpha1, cosU1 * cosSigma - sinU1 * sinSigma * cosAlpha1);
      double C = (f / 16.0) * cos2Alpha * (4.0 + f * (4.0 - 3.0 * cos2Alpha));
      double L = lambda - (1.0 - C) * f * sinAlpha * (sigma + C * sinSigma * (cosSigmaM2 + C * cosSigma * (-1.0 + 2.0 * cosSigmaM2 * cosSigmaM2)));
      return GeoCoord(phi2 * 180.0 / M_PI, ((lon * M_PI / 180.0) + L) * 180.0 / M_PI);
  }
}
