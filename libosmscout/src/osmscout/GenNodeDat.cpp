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

#include <osmscout/GenNodeDat.h>

#include <cmath>
#include <iostream>
#include <map>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Tiles.h>
#include <osmscout/RawNode.h>
#include <osmscout/Node.h>

namespace osmscout {

  bool GenerateNodeDat(const ImportParameter& parameter,
                       Progress& progress)
  {
    double minLon=-10.0;
    double minLat=-10.0;
    double maxLon=10.0;
    double maxLat=10.0;
    size_t count=0;

    //
    // Iterator over all raw nodes, hcekc they type, and convert them from raw nodes
    // to nodes if the type is interesting (!=typeIgnore).
    //
    // Count the bounding box by the way...
    //

    progress.SetAction("Generating nodes.dat");

    FileScanner scanner;
    FileWriter  writer;

    if (!scanner.Open("rawnodes.dat")) {
      progress.Error("Cannot open 'rawnodes.dat'");
      return false;
    }

    if (!writer.Open("nodes.dat")) {
      progress.Error("Cannot create 'nodes.dat'");
      return false;
    }

    while (!scanner.HasError()) {
      RawNode rawNode;
      Node    node;

      rawNode.Read(scanner);

      if (count==0) {
        minLat=rawNode.lat;
        minLon=rawNode.lon;
        maxLat=rawNode.lat;
        maxLon=rawNode.lon;
      }
      else {
        minLat=std::min(minLat,rawNode.lat);
        minLon=std::min(minLon,rawNode.lon);
        maxLat=std::max(maxLat,rawNode.lat);
        maxLon=std::max(maxLon,rawNode.lon);
      }

      if (rawNode.type!=typeIgnore) {
        node.id=rawNode.id;
        node.type=rawNode.type;
        node.lat=rawNode.lat;
        node.lon=rawNode.lon;
        node.tags=rawNode.tags;

        node.Write(writer);
      }

      count++;
    }

    scanner.Close();
    writer.Close();

    progress.SetAction("Generating bounding.dat");

    if (!writer.Open("bounding.dat")) {
      progress.Error("Cannot create 'bounding.dat'");
      return false;
    }

    // TODO: Dump bounding box to debug

    unsigned long minLatDat=round((minLat+180.0)*conversionFactor);
    unsigned long minLonDat=round((minLon+90.0)*conversionFactor);
    unsigned long maxLatDat=round((maxLat+180.0)*conversionFactor);
    unsigned long maxLonDat=round((maxLon+90.0)*conversionFactor);

    writer.WriteNumber(minLatDat);
    writer.WriteNumber(minLonDat);
    writer.WriteNumber(maxLatDat);
    writer.WriteNumber(maxLonDat);

    writer.Close();

    return true;
  }
}

