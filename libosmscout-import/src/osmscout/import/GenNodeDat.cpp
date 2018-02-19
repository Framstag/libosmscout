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

#include <osmscout/import/GenNodeDat.h>

#include <iostream>
#include <map>

#include <osmscout/GeoCoord.h>
#include <osmscout/Node.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/Preprocess.h>

namespace osmscout {

  const char* NodeDataGenerator::NODES_TMP="nodes.tmp";

  void NodeDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                         ImportModuleDescription& description) const
  {
    description.SetName("NodeDataGenerator");
    description.SetDescription("Convert raw nodes to nodes");

    description.AddRequiredFile(Preprocess::RAWNODES_DAT);

    description.AddProvidedTemporaryFile(NODES_TMP);
  }

  bool NodeDataGenerator::Import(const TypeConfigRef& typeConfig,
                                 const ImportParameter& parameter,
                                 Progress& progress)
  {
    uint32_t rawNodeCount=0;
    uint32_t nodesReadCount=0;
    uint32_t nodesWrittenCount=0;

    //
    // Iterator over all raw nodes, hcekc they type, and convert them from raw nodes
    // to nodes if the type is interesting (!=typeIgnore).
    //
    // Count the bounding box by the way...
    //

    progress.SetAction("Generating nodes.tmp");

    FileScanner scanner;
    FileWriter  writer;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWNODES_DAT),
                   FileScanner::Sequential,
                   parameter.GetRawNodeDataMemoryMaped());

      scanner.Read(rawNodeCount);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  NODES_TMP));

      writer.Write(nodesWrittenCount);

      for (uint32_t n=1; n<=rawNodeCount; n++) {
        progress.SetProgress(n,rawNodeCount);

        RawNode rawNode;
        Node    node;

        rawNode.Read(*typeConfig,
                     scanner);

        nodesReadCount++;

        if (rawNode.GetType()->GetIgnore()) {
          continue;
        }

        node.SetFeatures(rawNode.GetFeatureValueBuffer());
        node.SetCoords(rawNode.GetCoords());

        writer.Write((uint8_t)osmRefNode);
        writer.Write(rawNode.GetId());
        node.Write(*typeConfig,
                   writer);

        nodesWrittenCount++;
      }

      scanner.Close();

      writer.SetPos(0);
      writer.Write(nodesWrittenCount);
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    progress.Info(std::string("Read "+std::to_string(nodesReadCount)+" nodes, wrote "+std::to_string(nodesWrittenCount)+" nodes"));

    return true;
  }
}

