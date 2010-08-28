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
#include <osmscout/Util.h>

namespace osmscout {

  std::string NodeDataGenerator::GetDescription() const
  {
    return "Generate 'nodes.dat'";
  }

  bool NodeDataGenerator::Import(const ImportParameter& parameter,
                                 Progress& progress,
                                 const TypeConfig& typeConfig)
  {
    double   minLon=-10.0;
    double   minLat=-10.0;
    double   maxLon=10.0;
    double   maxLat=10.0;
    uint32_t rawNodeCount=0;
    uint32_t nodesReadCount=0;
    uint32_t nodesWrittenCount=0;

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

    if (!scanner.Read(rawNodeCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open("nodes.dat")) {
      progress.Error("Cannot create 'nodes.dat'");
      return false;
    }

    writer.Write(nodesWrittenCount);

    for (uint32_t n=1; n<=rawNodeCount; n++) {
      RawNode rawNode;
      Node    node;

      if (!rawNode.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(rawNodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (nodesReadCount==0) {
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

      nodesReadCount++;

      if (rawNode.type!=typeIgnore) {
        node.id=rawNode.id;
        node.type=rawNode.type;
        node.lat=rawNode.lat;
        node.lon=rawNode.lon;
        node.tags=rawNode.tags;

        node.Write(writer);
        nodesWrittenCount++;
      }

    }

    if (!scanner.Close()) {
      return false;
    }

    writer.SetPos(0);
    writer.Write(nodesWrittenCount);

    if (!writer.Close()) {
      return false;
    }

    progress.Info(std::string("Read "+NumberToString(nodesReadCount)+" nodes, wrote "+NumberToString(nodesWrittenCount)+" nodes"));

    progress.SetAction("Generating bounding.dat");

    if (!writer.Open("bounding.dat")) {
      progress.Error("Cannot create 'bounding.dat'");
      return false;
    }

    // TODO: Dump bounding box to debug

    uint32_t minLatDat=round((minLat+90.0)*conversionFactor);
    uint32_t minLonDat=round((minLon+180.0)*conversionFactor);
    uint32_t maxLatDat=round((maxLat+90.0)*conversionFactor);
    uint32_t maxLonDat=round((maxLon+180.0)*conversionFactor);

    writer.WriteNumber(minLatDat);
    writer.WriteNumber(minLonDat);
    writer.WriteNumber(maxLatDat);
    writer.WriteNumber(maxLonDat);

    writer.Close();

    return true;
  }
}

