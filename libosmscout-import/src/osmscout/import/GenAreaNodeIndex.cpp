/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/import/GenAreaNodeIndex.h>

#include <map>
#include <numeric>
#include <vector>

#include <osmscout/Node.h>
#include <osmscout/Pixel.h>

#include <osmscout/AreaNodeIndex.h>
#include <osmscout/NodeDataFile.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Number.h>

namespace osmscout {

  AreaNodeIndexGenerator::TypeData::TypeData()
  : level(0),
    cellCount(0),
    nodeCount(0),
    tileBox(TileId(0,0),
            TileId(0,0)),
    indexOffset(0)
  {
  }

  void AreaNodeIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                            ImportModuleDescription& description) const
  {
    description.SetName("AreaNodeIndexGenerator");
    description.SetDescription("Index nodes for area lookup");

    description.AddRequiredFile(NodeDataFile::NODES_DAT);

    description.AddProvidedFile(AreaNodeIndex::AREA_NODE_IDX);
  }

  bool AreaNodeIndexGenerator::ScanningNodeData(const TypeConfigRef& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                std::vector<TypeData>& nodeTypeData)
  {
    FileScanner nodeScanner;
    FileWriter  writer;
    TypeInfoSet remainingNodeTypes; //! Set of types we still must process

    nodeTypeData.resize(typeConfig->GetTypeCount());

    try {
      MagnificationLevel level;

      nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       NodeDataFile::NODES_DAT),
                       FileScanner::Sequential,
                       true);

      //
      // Scanning distribution
      //

      progress.SetAction("Scanning level distribution of node types");

      // Initially we must process all types that represents nodes and that should
      // not be ignored
      remainingNodeTypes.Set(typeConfig->GetNodeTypes());

      level=parameter.GetAreaNodeMinMag();
      while (!remainingNodeTypes.Empty()) {
        uint32_t                             dataCount=0;
        TypeInfoSet                          currentNodeTypes(remainingNodeTypes);
        std::vector<std::map<TileId,size_t>> cellFillCount; // Number of nodes per type and per tile in current level
        std::vector<GeoBox>                  boundingBoxes;

        // Reserve memory
        boundingBoxes.resize(typeConfig->GetTypeCount());
        cellFillCount.resize(typeConfig->GetTypeCount());

        progress.Info("Scanning Level "+std::to_string(level.Get())+" ("+std::to_string(remainingNodeTypes.Size())+
                      " types still to process)");

        nodeScanner.GotoBegin();
        nodeScanner.Read(dataCount);

        for (uint32_t n=1; n<=dataCount; n++) {
          progress.SetProgress(n,dataCount);

          Node node;

          node.Read(*typeConfig,
                    nodeScanner);

          // If we still need to handle this type,
          // count number of entries per type and tile cell
          if (currentNodeTypes.IsSet(node.GetType())) {
            size_t typeIndex=node.GetType()->GetIndex();

            boundingBoxes[typeIndex].Include(node.GetCoords());
            cellFillCount[typeIndex][TileId::GetTile(level,node.GetCoords())]++;
          }
        }

        // Check statistics for each type
        // If statistics are within goal limits, use this level
        // for this type (else try again with the next higher level)
        for (const auto& type : currentNodeTypes) {
          TypeData&                typeData=nodeTypeData[type->GetIndex()];
          GeoBox&                  boundingBox=boundingBoxes[type->GetIndex()];
          std::map<TileId,size_t>& tillFillCounts=cellFillCount[type->GetIndex()];

          typeData.type=type;
          typeData.level=level;
          typeData.cellCount=tillFillCounts.size();
          typeData.nodeCount=0;

          // If we do not have any entries, we store it now
          if (tillFillCounts.empty()) {
            continue;
          }

          typeData.tileBox=TileIdBox(TileId::GetTile(level,boundingBox.GetMinCoord()),
                                     TileId::GetTile(level,boundingBox.GetMaxCoord()));

          // Statistics

          for (const auto& cell : tillFillCounts) {
            typeData.nodeCount+=cell.second;
          }

          // Count absolute number of entries
          size_t maxTileCount=0;

          for (const auto& cell : tillFillCounts) {
            maxTileCount=std::max(maxTileCount,cell.second);
          }

          // Average number of entries per tile cell
          double averageTileCount=typeData.nodeCount*1.0/tillFillCounts.size();
          double fillRate=typeData.cellCount/(1.0*typeData.tileBox.GetCount());

          // Decide...

          // If the fill rate of the index is too low, we use this index level anyway
          if (fillRate<=parameter.GetAreaNodeIndexMinFillRate()) {
            progress.Warning(type->GetName()+" ("+std::to_string(type->GetIndex())+") is not well distributed");
            continue;
          }

          // If averageTileCount fill size and maxTileCount fill size for tile cells
          // is within limits, store it now.
          if (maxTileCount<=parameter.GetAreaNodeIndexCellSizeMax() &&
              averageTileCount<=parameter.GetAreaNodeIndexCellSizeAverage()) {
            continue;
          }

          // else, we remove it from the list and try again with an higher
          // level.
          currentNodeTypes.Remove(type);
        }

        remainingNodeTypes.Remove(currentNodeTypes);
        level++;
      }

      nodeScanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void AreaNodeIndexGenerator::DumpNodeData(Progress& progress,
                                            const std::vector<TypeData>& nodeTypeData)
  {
    MagnificationLevel minLevel(std::numeric_limits<uint32_t>::max());
    MagnificationLevel maxLevel(0);

    progress.SetAction("Dumping index structure");


    for (const auto& type : nodeTypeData) {
      if (type.HasEntries()) {
        minLevel=std::min(minLevel,type.level);
        maxLevel=std::max(maxLevel,type.level);
      }
    }

    for (MagnificationLevel l=minLevel; l<=maxLevel; ++l) {
      progress.Info("Index level "+std::to_string(l.Get())+":");
      for (const auto& typeData : nodeTypeData) {
        if (typeData.level!=l || !typeData.HasEntries()) {
          continue;
        }

        progress.Info("- "+typeData.type->GetName()+"("+std::to_string(typeData.type->GetIndex())+"), "+
                      std::to_string(typeData.cellCount)+" cells, "+
                      std::to_string(typeData.nodeCount)+" objects");
      }
    }
  }

  bool AreaNodeIndexGenerator::WriteIndexFile(const TypeConfigRef& typeConfig,
                                              const ImportParameter& parameter,
                                              Progress& progress,
                                              std::vector<TypeData>& nodeTypeData)
  {
    FileScanner        nodeScanner;
    FileWriter         writer;
    MagnificationLevel minLevel(std::numeric_limits<uint32_t>::max());
    MagnificationLevel maxLevel(0);

    progress.SetAction("Generating 'areanode.idx'");

    for (const auto& type : nodeTypeData) {
      if (type.HasEntries()) {
        minLevel=std::min(minLevel,type.level);
        maxLevel=std::max(maxLevel,type.level);
      }
    }

    try {
      nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       NodeDataFile::NODES_DAT),
                       FileScanner::Sequential,
                       true);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AreaNodeIndex::AREA_NODE_IDX));

      // Count number of types in index
      uint32_t indexEntryCount=std::accumulate(typeConfig->GetNodeTypes().begin(),
                                               typeConfig->GetNodeTypes().end(),
                                               uint32_t(0u),
                                               [&nodeTypeData](uint32_t value, const TypeInfoRef& type) {
                                                 if (nodeTypeData[type->GetIndex()].HasEntries()) {
                                                   value++;
                                                 }

                                                 return value;
      });

      writer.Write(indexEntryCount);

      // Store index data for each type
      for (auto& typeData : nodeTypeData) {
        if (!typeData.HasEntries()) {
          continue;
        }

        FileOffset bitmapOffset=0;
        uint8_t dataOffsetBytes=0;

        writer.WriteNumber(typeData.type->GetNodeId());

        typeData.indexOffset=writer.GetPos();

        writer.WriteFileOffset(bitmapOffset);
        writer.Write(dataOffsetBytes);

        writer.WriteNumber(typeData.level.Get());
        writer.WriteNumber(typeData.tileBox.GetMinX());
        writer.WriteNumber(typeData.tileBox.GetMaxX());
        writer.WriteNumber(typeData.tileBox.GetMinY());
        writer.WriteNumber(typeData.tileBox.GetMaxY());
      }

      // Now store index bitmap for each type in increasing level order (why?)
      for (MagnificationLevel l=minLevel; l<=maxLevel; ++l) {
        std::set<TypeInfoRef> indexTypes;
        uint32_t              nodeCount;

        for (const auto& typeData : nodeTypeData) {
          if (typeData.HasEntries() &&
              typeData.level==l) {
            indexTypes.insert(typeData.type);
          }
        }

        if (indexTypes.empty()) {
          continue;
        }

        progress.Info("Index level "+std::to_string(l.Get())+":");

        std::vector<std::map<TileId,std::list<FileOffset> > > typeBitmapData;

        typeBitmapData.resize(typeConfig->GetTypeCount());

        nodeScanner.GotoBegin();

        nodeScanner.Read(nodeCount);

        //
        // Collect all node offsets for each bitmap cell for all current types
        //
        for (uint32_t n=1; n<=nodeCount; n++) {
          progress.SetProgress(n,nodeCount);

          FileOffset offset;
          Node       node;

          offset=nodeScanner.GetPos();

          node.Read(*typeConfig,
                    nodeScanner);

          auto      typeIndex=node.GetType()->GetIndex();
          TypeData& typeData=nodeTypeData[typeIndex];

          if (typeData.level==l &&
              typeData.HasEntries()) {
            TileId tileId=TileId::GetTile(l,node.GetCoords());

            typeBitmapData[typeIndex][tileId].push_back(offset);
          }
        }

        //
        // Write bitmap for each current node type
        //
        for (const auto& type : indexTypes) {
          size_t indexEntries=0;
          size_t dataSize=0;
          char   buffer[10];

          // Calculate the size of the bitmap for each current node type
          for (const auto& cell : typeBitmapData[type->GetIndex()]) {
            indexEntries+=cell.second.size();

            dataSize+=EncodeNumber(cell.second.size(),buffer);

            FileOffset previousOffset=0;
            for (auto offset : cell.second) {
              FileOffset data=offset-previousOffset;

              dataSize+=EncodeNumber(data,buffer);

              previousOffset=offset;
            }
          }

          // "+1" because we add +1 to every offset, to generate offset > 0
          uint8_t dataOffsetBytes=BytesNeededToEncodeNumber(dataSize);

          progress.Info("- "+
                        type->GetName()+", "+
                        std::to_string(typeBitmapData[type->GetIndex()].size())+" cells, "+
                        std::to_string(indexEntries)+" entries, "+
                        ByteSizeToString(1.0*dataOffsetBytes*nodeTypeData[type->GetIndex()].tileBox.GetCount()));

          // Remember start of the bitmap
          FileOffset bitmapOffset=writer.GetPos();

          assert(nodeTypeData[type->GetIndex()].indexOffset!=0);

          // Write bitmap offset and data size to the index
          writer.SetPos(nodeTypeData[type->GetIndex()].indexOffset);
          writer.WriteFileOffset(bitmapOffset);
          writer.Write(dataOffsetBytes);
          writer.SetPos(bitmapOffset);

          // Write the bitmap with offsets for each cell
          // We fill the bitmap on disk with zero and only overwrite cells later that have data
          // So zero means "no data for this cell"
          for (size_t i=0; i<nodeTypeData[type->GetIndex()].tileBox.GetCount(); i++) {
            FileOffset cellOffset=0;

            writer.WriteFileOffset(cellOffset,
                                   dataOffsetBytes);
          }

          // Remember the start of the data section behind the bitmap itself for delta offset calculation later
          FileOffset dataStartOffset=writer.GetPos();

          // Now write the list of offsets for nodes for every cell with content
          for (const auto& cell : typeBitmapData[type->GetIndex()]) {
            FileOffset bitmapCellOffset=bitmapOffset+
                                        ((cell.first.GetY()-nodeTypeData[type->GetIndex()].tileBox.GetMinY())*
                                         nodeTypeData[type->GetIndex()].tileBox.GetWidth()+
                                         cell.first.GetX()-nodeTypeData[type->GetIndex()].tileBox.GetMinX())*dataOffsetBytes;
            FileOffset previousOffset=0;
            FileOffset cellOffset=writer.GetPos();

            // Write the offset the node offset list of the current cell to the bitmap
            writer.SetPos(bitmapCellOffset);
            writer.WriteFileOffset(cellOffset-dataStartOffset+1,
                                   dataOffsetBytes);
            writer.SetPos(cellOffset);

            // Write the number of node offsets and the node offsets itself using offset deltas
            writer.WriteNumber((uint32_t)cell.second.size());
            for (auto offset : cell.second) {
              writer.WriteNumber((FileOffset)(offset-previousOffset));

              previousOffset=offset;
            }
          }
        }
      }

      nodeScanner.Close();
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool AreaNodeIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    std::vector<TypeData> nodeTypeData;

    if (!ScanningNodeData(typeConfig,
                          parameter,
                          progress,
                          nodeTypeData)) {
      return false;
    }

    DumpNodeData(progress,
                 nodeTypeData);

    return WriteIndexFile(typeConfig,
                          parameter,
                          progress,
                          nodeTypeData);
  }
}
