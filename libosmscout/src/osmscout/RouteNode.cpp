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

#include <osmscout/RouteNode.h>

#include <limits>

#include <osmscout/private/Math.h>

namespace osmscout {

  bool RouteNode::Read(FileScanner& scanner)
  {
    uint32_t pathCount;
    uint32_t excludesCount;
    uint32_t minLat;
    uint32_t minLon;

    scanner.Read(id);
    scanner.ReadNumber(pathCount);
    scanner.ReadNumber(excludesCount);

    scanner.Read(minLat);
    scanner.Read(minLon);

    if (scanner.HasError()) {
      return false;
    }

    paths.resize(pathCount);
    for (size_t i=0; i<pathCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;
      uint32_t distanceValue;

      scanner.Read(paths[i].id);
      scanner.ReadNumber(paths[i].type);
      scanner.Read(paths[i].wayId);
      scanner.ReadNumber(distanceValue);
      scanner.ReadNumber(latValue);
      scanner.ReadNumber(lonValue);

      paths[i].distance=distanceValue/(1000.0*100.0);
      paths[i].lat=(latValue+minLat)/conversionFactor-90.0;
      paths[i].lon=(lonValue+minLon)/conversionFactor-180.0;
    }

    excludes.resize(excludesCount);
    for (size_t i=0; i<excludesCount; i++) {
      scanner.Read(excludes[i].sourceWay);
      scanner.ReadNumber(excludes[i].targetPath);
    }

    return !scanner.HasError();
  }

  bool RouteNode::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber((uint32_t)paths.size());
    writer.WriteNumber((uint32_t)excludes.size());

    uint32_t minLat=std::numeric_limits<uint32_t>::max();
    uint32_t minLon=std::numeric_limits<uint32_t>::max();

    for (size_t i=0; i<paths.size(); i++) {
      minLat=std::min(minLat,(uint32_t)floor((paths[i].lat+90.0)*conversionFactor+0.5));
      minLon=std::min(minLon,(uint32_t)floor((paths[i].lon+180.0)*conversionFactor+0.5));
    }

    writer.Write(minLat);
    writer.Write(minLon);

    for (size_t i=0; i<paths.size(); i++) {
      uint32_t latValue=(uint32_t)floor((paths[i].lat+90.0)*conversionFactor+0.5);
      uint32_t lonValue=(uint32_t)floor((paths[i].lon+180.0)*conversionFactor+0.5);
      uint32_t distanceValue=(uint32_t)floor(paths[i].distance*(1000.0*100.0)+0.5);

      writer.Write(paths[i].id);
      writer.WriteNumber(paths[i].type);
      writer.Write(paths[i].wayId);
      writer.WriteNumber(distanceValue);
      writer.WriteNumber(latValue-minLat);
      writer.WriteNumber(lonValue-minLon);
    }

    for (size_t i=0; i<excludes.size(); i++) {
      writer.Write(excludes[i].sourceWay);
      writer.WriteNumber(excludes[i].targetPath);
    }

    return !writer.HasError();
  }

}

