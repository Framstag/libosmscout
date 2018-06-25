/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Magnification.h>

namespace osmscout {

  MagnificationLevel Magnification::magWorld(0);
  MagnificationLevel Magnification::magContinent(4);
  MagnificationLevel Magnification::magState(5);
  MagnificationLevel Magnification::magStateOver(6);
  MagnificationLevel Magnification::magCounty(7);
  MagnificationLevel Magnification::magRegion(8);
  MagnificationLevel Magnification::magProximity(9);
  MagnificationLevel Magnification::magCityOver(10);
  MagnificationLevel Magnification::magCity(11);
  MagnificationLevel Magnification::magSuburb(12);
  MagnificationLevel Magnification::magDetail(13);
  MagnificationLevel Magnification::magClose(14);
  MagnificationLevel Magnification::magCloser(15);
  MagnificationLevel Magnification::magVeryClose(16);
  MagnificationLevel Magnification::magBlock(18);
  MagnificationLevel Magnification::magStreet(19);
  MagnificationLevel Magnification::magHouse(20);

  void Magnification::SetMagnification(double magnification)
  {
    this->magnification=magnification;
    this->level=(uint32_t)log2(this->magnification);
  }

  void Magnification::SetLevel(const MagnificationLevel& level)
  {
    this->magnification=pow(2.0,level.Get());
    this->level=level.Get();
  }

  MagnificationConverter::MagnificationConverter()
  {
    stringToMagMap[std::string("world")]=Magnification::magWorld;
    stringToMagMap[std::string("continent")]=Magnification::magContinent;
    stringToMagMap[std::string("state")]=Magnification::magState;
    stringToMagMap[std::string("stateOver")]=Magnification::magStateOver;
    stringToMagMap[std::string("county")]=Magnification::magCounty;
    stringToMagMap[std::string("region")]=Magnification::magRegion;
    stringToMagMap[std::string("proximity")]=Magnification::magProximity;
    stringToMagMap[std::string("cityOver")]=Magnification::magCityOver;
    stringToMagMap[std::string("city")]=Magnification::magCity;
    stringToMagMap[std::string("suburb")]=Magnification::magSuburb;
    stringToMagMap[std::string("detail")]=Magnification::magDetail;
    stringToMagMap[std::string("close")]=Magnification::magClose;
    stringToMagMap[std::string("closer")]=Magnification::magCloser;
    stringToMagMap[std::string("veryClose")]=Magnification::magVeryClose;
    stringToMagMap[std::string("block")]=Magnification::magBlock;
    stringToMagMap[std::string("street")]=Magnification::magStreet;
    stringToMagMap[std::string("house")]=Magnification::magHouse;

    levelToStringMap[Magnification::magWorld]="world";
    levelToStringMap[Magnification::magContinent] = "continent";
    levelToStringMap[Magnification::magState] = "state";
    levelToStringMap[Magnification::magStateOver] = "stateOver";
    levelToStringMap[Magnification::magCounty] = "county";
    levelToStringMap[Magnification::magRegion] = "region";
    levelToStringMap[Magnification::magProximity] = "proximity";
    levelToStringMap[Magnification::magCityOver] = "cityOver";
    levelToStringMap[Magnification::magCity] = "city";
    levelToStringMap[Magnification::magSuburb] = "suburb";
    levelToStringMap[Magnification::magDetail] = "detail";
    levelToStringMap[Magnification::magClose] = "close";
    levelToStringMap[Magnification::magCloser] = "closer";
    levelToStringMap[Magnification::magVeryClose] = "veryClose";
    levelToStringMap[Magnification::magBlock] = "block";
    levelToStringMap[Magnification::magStreet] = "street";
    levelToStringMap[Magnification::magHouse] = "house";
  }

  bool MagnificationConverter::Convert(const std::string& name,
                                       Magnification& magnification)
  {
    auto entry=stringToMagMap.find(name);

    if (entry==stringToMagMap.end()) {
      return false;
    }

    magnification.SetLevel(entry->second);

    return true;
  }

  bool MagnificationConverter::Convert(const MagnificationLevel& level,
                                       std::string& name)
  {
    auto entry=levelToStringMap.find(level);

    if (entry==levelToStringMap.end()) {
      return false;
    }

    name=entry->second;

    return true;
  }
}
