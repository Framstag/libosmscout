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

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Tiling.h>

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

  double GeoCoord::GetDistance(GeoCoord target) const
  {
      return GetEllipsoidalDistance(*this, target);
  }

  GeoCoord GeoCoord::Add(double bearing, double distance)
  {
      if (distance == 0.0) return GeoCoord(GetLat(), GetLon());
      double lat = GetLat();
      double lon = GetLon();
      GetEllipsoidalDistance(GetLat(), GetLon(), bearing, distance, lat, lon);
      return GeoCoord(lat, lon);
  }
}
