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

#include <numeric>

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

  /**
   * Count the number of types for that we have found data for.
   *
   * @param data
   *    DistributionData
   * @return
   *    Number of types with data
   */
  static uint16_t CountIndexTypes(const std::vector<AreaNodeIndexGenerator::DistributionData>& data)
  {
    return std::accumulate(data.begin(),
                           data.end(),
                           uint16_t(0u),
                           [](uint16_t value,
                              const AreaNodeIndexGenerator::DistributionData& entry) {
                             if (entry.fillCount>0) {
                               value++;
                             }

                             return value;
                           });
  }

  /**
   * Count the number of tiles of index type "list" over all types
   *
   * @param data
   *    DistributionData
   * @return
   *    Number of tiles
   */
  static uint32_t CountListTiles(const std::vector<AreaNodeIndexGenerator::DistributionData>& data)
  {
    return std::accumulate(data.begin(),
                           data.end(),
                           (uint32_t) 0u,
                           [](uint32_t value,
                              const AreaNodeIndexGenerator::DistributionData& entry) {
                             return value+entry.listTiles.size();
                           });
  }

  /**
   * Count the number of tiles of index type "bitmap" over all types
   *
   * @param data
   *    DistributionData
   * @return
   *    Number of tiles
   */
  static uint32_t CountBitmapTiles(const std::vector<AreaNodeIndexGenerator::DistributionData>& data)
  {
    return std::accumulate(data.begin(),
                           data.end(),
                           (uint32_t) 0u,
                           [](uint32_t value,
                              const AreaNodeIndexGenerator::DistributionData& entry) {
                             return value+entry.bitmapTiles.size();
                           });
  }

  static MagnificationLevel GetBitmapZoomLevel(const ImportParameter& parameter,
                                               const std::list<std::pair<GeoCoord,FileOffset>>& data)
  {
    MagnificationLevel magnification=parameter.GetAreaNodeGridMag();

    //Bitmap should al least have this size to make sense
    magnification++;

    // That is our maximum magnification, beyond that we trade disk size in favour of performance
    while (magnification<parameter.GetAreaNodeBitmapMaxMag()) {
      std::map<TileId,std::list<FileOffset>> bitmapData;

      for (const auto& entry : data) {
        TileId dataTileId=TileId::GetTile(magnification,entry.first);

        bitmapData[dataTileId].push_back(entry.second);
      }

      if (std::all_of(bitmapData.begin(),
                      bitmapData.end(),
                      [&parameter](const std::pair<TileId,std::list<FileOffset>>& entry) {
                        return entry.second.size()<=parameter.GetAreaNodeBitmapLimit();
      })) {
        return magnification;
      }

      magnification++;
    }

    return magnification;
  }

  void AreaNodeIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                            ImportModuleDescription& description) const
  {
    description.SetName("AreaNodeIndexGenerator");
    description.SetDescription("Index nodes for area lookup");

    description.AddRequiredFile(NodeDataFile::NODES_DAT);

    description.AddProvidedFile(AreaNodeIndex::AREA_NODE_IDX);
  }

  bool AreaNodeIndexGenerator::AnalyseDistribution(const TypeConfigRef& typeConfig,
                                                   const ImportParameter& parameter,
                                                   Progress& progress,
                                                   std::vector<DistributionData>& data)
  {
    data.resize(typeConfig->GetNodeTypes().size()+1);

    try {
      FileScanner        nodeScanner;

      nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       NodeDataFile::NODES_DAT),
                       FileScanner::Sequential,
                       true);

      //
      // Scanning distribution
      //

      progress.SetAction("Scanning distribution of node types");

      uint32_t dataCount=0;

      nodeScanner.Read(dataCount);

      for (uint32_t n=1; n<=dataCount; n++) {
        progress.SetProgress(n,dataCount);

        Node node;

        node.Read(*typeConfig,
                  nodeScanner);

        TypeId typeId=node.GetType()->GetNodeId();

        data[typeId].type=node.GetType();
        data[typeId].nodeId=typeId;
        data[typeId].boundingBox.Include(node.GetCoords());
        data[typeId].tileFillCount[TileId::GetTile(parameter.GetAreaNodeGridMag(),node.GetCoords())]++;
      }

      nodeScanner.Close();

      progress.SetAction("Analysing distribution of node types");

      for (auto& entry : data) {
        entry.fillCount=std::accumulate(entry.tileFillCount.begin(),
                                        entry.tileFillCount.end(),
                                        (size_t)0,
                                        [](size_t sum, std::pair<const osmscout::TileId,size_t> current) {
                                          return sum+current.second;
                                        });

        if (entry.fillCount>parameter.GetAreaNodeSimpleListLimit()) {
          entry.isComplex=true;
          std::for_each(entry.tileFillCount.begin(),
                        entry.tileFillCount.end(),
                        [&entry,&parameter](const std::pair<TileId,size_t>& tileData) {
                          if (tileData.second<=parameter.GetAreaNodeTileListLimit()) {
                            entry.listTiles.insert(tileData.first);
                          }
                          else {
                            entry.bitmapTiles.insert(tileData.first);
                          }
          });
        }
        else {
          entry.isComplex=false;
        }
      }
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void AreaNodeIndexGenerator::DumpDistribution(Progress& progress,
                                                const std::vector<DistributionData>& data)
  {
    progress.Info("Distribution data:");
    for (const auto& entry : data) {
      if (entry.HasNoData()) {
        continue;
      }

      if (!entry.IsComplexIndex()) {
        progress.Info("- "+entry.type->GetName()+"("+std::to_string(entry.type->GetIndex())+"), "+
                      std::to_string(entry.fillCount)+" nodes");
      }
      else if (!entry.listTiles.empty() &&
               entry.bitmapTiles.empty()) {
        progress.Info("- "+entry.type->GetName()+"("+std::to_string(entry.type->GetIndex())+"), "+
                      std::to_string(entry.listTiles.size())+" list tiles, "+
                      std::to_string(entry.fillCount)+" nodes");
      }
      else if (entry.listTiles.empty() &&
               !entry.bitmapTiles.empty()) {
        progress.Info("- "+entry.type->GetName()+"("+std::to_string(entry.type->GetIndex())+"), "+
                      std::to_string(entry.bitmapTiles.size())+" bitmap tiles, "+
                      std::to_string(entry.fillCount)+" nodes");
      }
      else {
        progress.Info("- "+entry.type->GetName()+"("+std::to_string(entry.type->GetIndex())+"), "+
                      std::to_string(entry.listTiles.size())+" list tiles, "+
                      std::to_string(entry.bitmapTiles.size())+" bitmap tiles, "+
                      std::to_string(entry.fillCount)+" nodes");
      }
    }
  }

  /**
   * Write the list of indexed types together with information about the type of index used.
   *
   * @param progress
   *    Progress object
   * @param data
   *    Information about the distribution of data
   * @param writer
   *    File writer for the index
   * @return
   *    List of file offsets pointing into the individual index, where later on data needs to get patched.
   */
  std::vector<FileOffset> AreaNodeIndexGenerator::WriteListIndex(Progress& progress,
                                                                 const std::vector<DistributionData>& data,
                                                                 FileWriter& writer)
  {
    std::vector<FileOffset> listIndexOffsets(data.size());

    // Count number of types in index
    uint16_t indexEntryCount=CountIndexTypes(data);

    progress.Info("Writing "+std::to_string(indexEntryCount)+" list index entries");

    writer.Write(indexEntryCount);
    for (const auto& entry : data) {
      // Skip types without data
      if (entry.HasNoData()) {
        continue;
      }

      writer.WriteNumber(entry.nodeId);
      writer.Write(entry.isComplex);
      writer.WriteCoord(entry.boundingBox.GetMinCoord());
      writer.WriteCoord(entry.boundingBox.GetMaxCoord());

      listIndexOffsets[entry.type->GetNodeId()]=writer.GetPos();

      if (!entry.IsComplexIndex()) {
        // data offset
        writer.WriteFileOffset((FileOffset)0);
        // number of entries in list
        writer.Write((uint16_t)0);
      }
    }

    return listIndexOffsets;
  }

  std::vector<std::map<TileId,FileOffset>> AreaNodeIndexGenerator::WriteTileListIndex(Progress& progress,
                                                                                      const std::vector<DistributionData>& data,
                                                                                      FileWriter& writer)
  {
    std::vector<std::map<TileId,FileOffset>> tileIndexOffsets(data.size());

    auto listTileCount=CountListTiles(data);

    // Number of entries to come
    progress.Info("Writing "+std::to_string(listTileCount)+" list tile index entries");
    writer.Write(listTileCount);

    for (const auto& entry : data) {
      for (const auto& tileId : entry.listTiles) {
        writer.WriteNumber(entry.nodeId);
        writer.Write(tileId.GetX());
        writer.Write(tileId.GetY());

        tileIndexOffsets[entry.nodeId][tileId]=writer.GetPos();

        // data offset
        writer.WriteFileOffset((FileOffset)0);
        // number of entries in list
        writer.Write((uint16_t)0);
        // We also store GeoCoord for faster indexing
        writer.Write(false);
      }
    }

    return tileIndexOffsets;
  }

  std::vector<std::map<TileId,FileOffset>> AreaNodeIndexGenerator::WriteBitmapIndex(Progress& progress,
                                                                                    const std::vector<DistributionData>& data,
                                                                                    FileWriter& writer)
  {
    std::vector<std::map<TileId,FileOffset>> bitmapIndexOffsets(data.size());

    auto bitmapTileCount=CountBitmapTiles(data);

    // Number of entries to come
    progress.Info("Writing "+std::to_string(bitmapTileCount)+" bitmap tile index entries");
    writer.Write(bitmapTileCount);

    for (const auto& entry : data) {
      for (const auto& tileId : entry.bitmapTiles) {
        writer.WriteNumber(entry.nodeId);
        writer.Write(tileId.GetX());
        writer.Write(tileId.GetY());

        bitmapIndexOffsets[entry.nodeId][tileId]=writer.GetPos();

        // bitmap offset
        writer.WriteFileOffset((FileOffset)0);
        // dataOffsetBytes
        writer.Write((uint8_t)0);
        // bitmap cell level
        writer.Write((uint8_t)0);
      }
    }

    return bitmapIndexOffsets;
  }

  void AreaNodeIndexGenerator::WriteListData(Progress& progress,
                                             const std::vector<DistributionData>& data,
                                             const std::vector<std::list<std::pair<GeoCoord,FileOffset>>>& listData,
                                             const std::vector<FileOffset>& listIndexOffsets,
                                             FileWriter& writer)
  {
    progress.Info("Writing list data");

    for (const auto& entry : data) {
      if (entry.HasNoData()) {
        continue;
      }

      if (entry.IsComplexIndex()) {
        continue;
      }

      auto indexStart=writer.GetPos();

      assert(listIndexOffsets[entry.nodeId]!=0);

      writer.SetPos(listIndexOffsets[entry.nodeId]);
      writer.WriteFileOffset(indexStart);
      writer.Write((uint16_t)listData[entry.nodeId].size());
      writer.SetPos(indexStart);

      FileOffset previousOffset=0;

      for (const auto& listEntry : listData[entry.nodeId]) {
        writer.WriteCoord(listEntry.first);
        writer.WriteNumber(listEntry.second-previousOffset);

        previousOffset=listEntry.second;
      }
    }
  }

  void AreaNodeIndexGenerator::WriteTileListData(const ImportParameter& parameter,
                                                 const DistributionData& distributionData,
                                                 const std::list<std::pair<GeoCoord,FileOffset>>& tileData,
                                                 const FileOffset& tileIndexOffset,
                                                 FileWriter& writer)
  {
    auto storeGeoCoord=distributionData.fillCount<parameter.GetAreaNodeTileListCoordLimit();
    auto indexOffset=writer.GetPos();

    writer.SetPos(tileIndexOffset);
    writer.WriteFileOffset(indexOffset);
    writer.Write((uint16_t)tileData.size());
    writer.Write(storeGeoCoord);
    writer.SetPos(indexOffset);

    FileOffset previousOffset=0;

    for (const auto& tileEntry : tileData) {
      if (storeGeoCoord) {
        writer.WriteCoord(tileEntry.first);
      }
      writer.WriteNumber(tileEntry.second-previousOffset);

      previousOffset=tileEntry.second;
    }
  }

  void AreaNodeIndexGenerator::WriteBitmapData(const ImportParameter& parameter,
                                               const TileId& tileId,
                                               const std::list<std::pair<GeoCoord,FileOffset>>& data,
                                               const FileOffset& bitmapIndexOffset,
                                               FileWriter& writer)
  {
    std::map<TileId,std::list<FileOffset>> bitmapData;
    size_t                                 indexEntries=0;
    size_t                                 dataSize=0;
    char                                   buffer[10];
    MagnificationLevel                     magnification=GetBitmapZoomLevel(parameter,data);

    for (const auto& entry : data) {
      TileId dataTileId=TileId::GetTile(magnification,entry.first);

      bitmapData[dataTileId].push_back(entry.second);
    }

    GeoBox    box=tileId.GetBoundingBox(Magnification(parameter.GetAreaNodeGridMag()));
    TileIdBox tileBox(TileId::GetTile(magnification,box.GetMinCoord()),
                      TileId::GetTile(magnification,box.GetMaxCoord()));

    // Calculate the size of the bitmap for each current node type
    for (const auto& cell : bitmapData) {
      indexEntries+=cell.second.size();

      dataSize+=EncodeNumber(cell.second.size(),buffer);

      FileOffset previousOffset=0;
      for (auto offset : cell.second) {
        FileOffset offsetDelta=offset-previousOffset;

        dataSize+=EncodeNumber(offsetDelta,buffer);

        previousOffset=offset;
      }
    }

    // "+1" because we add +1 to every offset, to generate offset > 0
    uint8_t dataOffsetBytes=BytesNeededToEncodeNumber(dataSize+1);

    // Remember start of the bitmap
    auto bitmapOffset=writer.GetPos();

    assert(bitmapIndexOffset!=0);

    // Write bitmap offset and data size to the index
    writer.SetPos(bitmapIndexOffset);
    writer.WriteFileOffset(bitmapOffset);
    writer.Write(dataOffsetBytes);
    writer.Write((uint8_t)magnification.Get());
    writer.SetPos(bitmapOffset);

    // Write the bitmap with offsets for each cell
    // We fill the bitmap on disk with zero and only overwrite cells later that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<tileBox.GetCount(); i++) {
      FileOffset cellOffset=0;

      writer.WriteFileOffset(cellOffset,
                             dataOffsetBytes);
    }

    // Remember the start of the data section behind the bitmap itself for delta offset calculation later
    auto dataStartOffset=writer.GetPos();

    // Now write the list of offsets for nodes for every cell with content
    for (const auto& cell : bitmapData) {
      FileOffset bitmapCellOffset=bitmapOffset+
                                  ((cell.first.GetY()-tileBox.GetMinY())*
                                   tileBox.GetWidth()+
                                   cell.first.GetX()-tileBox.GetMinX())*dataOffsetBytes;
      FileOffset previousOffset=0;
      auto       cellOffset=writer.GetPos();

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

  bool AreaNodeIndexGenerator::WriteData(const TypeConfigRef& typeConfig,
                                         const ImportParameter& parameter,
                                         Progress& progress,
                                         const std::vector<DistributionData>& data)
  {
    progress.SetAction("Generating 'areanode.idx'");

    try {
      FileScanner nodeScanner;
      FileWriter  writer;
      auto        level=parameter.GetAreaNodeGridMag();

      nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       NodeDataFile::NODES_DAT),
                       FileScanner::Sequential,
                       true);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AreaNodeIndex::AREA_NODE_IDX));

      writer.Write(parameter.GetAreaNodeGridMag().Get());

      //
      // Store list index data for each type
      //

      std::vector<FileOffset> listIndexOffsets=WriteListIndex(progress,data,writer);

      //
      // Store tile list index data for each type
      //

      std::vector<std::map<TileId,FileOffset>> tileIndexOffsets=WriteTileListIndex(progress,data,writer);

      //
      // Store bitmap index data for each type
      //

      std::vector<std::map<TileId,FileOffset>> bitmapIndexOffsets=WriteBitmapIndex(progress,data,writer);

      uint32_t nodeCount;

      //
      // Read list index data
      //

      std::vector<std::list<std::pair<GeoCoord,FileOffset>>> listData(data.size());

      nodeScanner.GotoBegin();
      nodeScanner.Read(nodeCount);

      //
      // Collect all node offsets for each bitmap cell for all current types
      //
      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        Node node;

        node.Read(*typeConfig,
                  nodeScanner);

        auto typeId=node.GetType()->GetNodeId();

        if (!data[typeId].IsComplexIndex()) {
          listData[typeId].push_back(std::make_pair(node.GetCoords(),node.GetFileOffset()));
        }
      }

      //
      // Write list data
      //

      WriteListData(progress,
                    data,
                    listData,
                    listIndexOffsets,
                    writer);

      listIndexOffsets.clear();
      listData.clear();

      //
      // Read tile list and bitmap index data
      //

      progress.Info("Writing tile list and bitmap index data");

      std::vector<std::map<TileId,std::list<std::pair<GeoCoord,FileOffset>>>> tileData(data.size());

      nodeScanner.GotoBegin();
      nodeScanner.Read(nodeCount);

      //
      // Collect all node offsets for each tile for all current types
      //
      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        Node node;

        node.Read(*typeConfig,
                  nodeScanner);

        auto typeId=node.GetType()->GetNodeId();

        if (!data[typeId].IsComplexIndex()) {
          continue;
        }

        auto tileId=TileId::GetTile(level,node.GetCoords());

        IndexType indexType;

        if (data[typeId].listTiles.find(tileId)!=data[typeId].listTiles.end()) {
          indexType=IndexType::IndexTypeList;
        }
        else if (data[typeId].bitmapTiles.find(tileId)!=data[typeId].bitmapTiles.end()) {
          indexType=IndexType::IndexTypeBitmap;
        }
        else {
          continue;
        }

        tileData[typeId][tileId].push_back(std::make_pair(node.GetCoords(),
                                                          node.GetFileOffset()));

        if (tileData[typeId][tileId].size()==data[typeId].tileFillCount.find(tileId)->second) {
          if (indexType==IndexType::IndexTypeList) {
            WriteTileListData(parameter,
                              data[typeId],
                              tileData[typeId][tileId],
                              tileIndexOffsets[typeId][tileId],
                              writer);
          }
          else {
            WriteBitmapData(parameter,
                            tileId,
                            tileData[typeId][tileId],
                            bitmapIndexOffsets[typeId][tileId],
                            writer);
          }

          tileData[typeId][tileId].clear();
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
    std::vector<DistributionData> distributionData;

    if (!AnalyseDistribution(typeConfig,
                             parameter,
                             progress,
                             distributionData)) {
      return false;
    }

    DumpDistribution(progress,
                     distributionData);


    return WriteData(typeConfig,
                     parameter,
                     progress,
                     distributionData);
  }
}
