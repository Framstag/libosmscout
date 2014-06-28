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

namespace osmscout {

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

  /**
   * Parse a textual representation of a geo coordinate from a string
   * to an GeoCoord instance.
   *
   * The text should follow the following expression:
   *
   * [+|-|N|S] DD[.DDDDD] [N|S] [+|-|W|E] DDD[.DDDDD] [W|E]
   *
   * The means:
   * * You first define the latitude, then the longitude value
   * * You can define with half you mean by either prefixing or postfixing
   * the actual number with a hint
   * * The hint can either be a sign ('-' or '+') or a direction ('N' and 'S'
   * for latitude, 'E' or 'W' for longitude).
   *
   *  Possibly in future more variants will be supported.
   *
   * @param text
   *    Text containing the textual representation
   * @param coord
   *    The resulting coordinate, if the text was correctly parsed
   * @return
   *    true, if the text was correctly parsed, else false
   */
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

  std::ostream& operator<<(std::ostream& stream, const GeoCoord& coord)
  {
    std::streamsize         oldPrecision=stream.precision(5);
    std::ios_base::fmtflags oldFlags=stream.setf(std::ios::fixed,std::ios::floatfield);

    stream << coord.GetLat();

    if (coord.GetLat()>=0) {
      stream << " N ";
    }
    else {
      stream << " S ";
    }

    stream << coord.GetLon();

    if (coord.GetLon()>=0) {
      stream << " E";
    }
    else {
      stream << " W";
    }

    stream.precision(oldPrecision);
    stream.setf(oldFlags,std::ios::floatfield);

    return stream;
  }

  const double latConversionFactor=134217727.0/180.0; // 27 Bit
  const double lonConversionFactor=134217727.0/360.0; // 27 Bit
  const size_t coordByteSize=7;

}
