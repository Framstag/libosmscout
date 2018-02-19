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
  : indexLevel(0),
    indexCells(0),
    indexEntries(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0),
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

  bool AreaNodeIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileScanner           nodeScanner;
    FileWriter            writer;
    TypeInfoSet           remainingNodeTypes; //! Set of types we still must process
    std::vector<TypeData> nodeTypeData;
    size_t                level;
    size_t                maxLevel=0;

    nodeTypeData.resize(typeConfig->GetTypeCount());

    try {
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
        uint32_t nodeCount=0;
        TypeInfoSet currentNodeTypes(remainingNodeTypes);
        std::vector<std::map<Pixel,size_t> > cellFillCount;

        cellFillCount.resize(typeConfig->GetTypeCount());

        progress.Info("Scanning Level "+std::to_string(level)+" ("+std::to_string(remainingNodeTypes.Size())+
                      " types still to process)");

        nodeScanner.GotoBegin();

        nodeScanner.Read(nodeCount);

        for (uint32_t n=1; n<=nodeCount; n++) {
          progress.SetProgress(n,nodeCount);

          Node node;

          node.Read(*typeConfig,
                    nodeScanner);

          // If we still need to handle this type,
          // count number of entries per type and tile cell
          if (currentNodeTypes.IsSet(node.GetType())) {
            uint32_t xc=(uint32_t) floor((node.GetCoords().GetLon()+180.0)/cellDimension[level].width);
            uint32_t yc=(uint32_t) floor((node.GetCoords().GetLat()+90.0)/cellDimension[level].height);

            cellFillCount[node.GetType()->GetIndex()][Pixel(xc,yc)]++;
          }
        }

        // Check statistics for each type
        // If statistics are within goal limits, use this level
        // for this type (else try again with the next higher level)
        for (const auto& type : currentNodeTypes) {
          size_t i=type->GetIndex();
          size_t entryCount=0;
          size_t max=0;

          nodeTypeData[i].indexLevel=(uint32_t) level;
          nodeTypeData[i].indexCells=cellFillCount[i].size();
          nodeTypeData[i].indexEntries=0;

          if (!cellFillCount[i].empty()) {
            nodeTypeData[i].cellXStart=cellFillCount[i].begin()->first.x;
            nodeTypeData[i].cellYStart=cellFillCount[i].begin()->first.y;

            nodeTypeData[i].cellXEnd=nodeTypeData[i].cellXStart;
            nodeTypeData[i].cellYEnd=nodeTypeData[i].cellYStart;

            for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount[i].begin();
                 cell!=cellFillCount[i].end();
                 ++cell) {
              nodeTypeData[i].indexEntries+=cell->second;

              nodeTypeData[i].cellXStart=std::min(nodeTypeData[i].cellXStart,cell->first.x);
              nodeTypeData[i].cellXEnd=std::max(nodeTypeData[i].cellXEnd,cell->first.x);

              nodeTypeData[i].cellYStart=std::min(nodeTypeData[i].cellYStart,cell->first.y);
              nodeTypeData[i].cellYEnd=std::max(nodeTypeData[i].cellYEnd,cell->first.y);
            }
          }

          nodeTypeData[i].cellXCount=nodeTypeData[i].cellXEnd-nodeTypeData[i].cellXStart+1;
          nodeTypeData[i].cellYCount=nodeTypeData[i].cellYEnd-nodeTypeData[i].cellYStart+1;

          // Count absolute number of entries
          for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount[i].begin();
               cell!=cellFillCount[i].end();
               ++cell) {
            entryCount+=cell->second;
            max=std::max(max,cell->second);
          }

          // Average number of entries per tile cell
          double average=entryCount*1.0/cellFillCount[i].size();

          // If we do not have any entries, we store it now
          if (cellFillCount[i].empty()) {
            continue;
          }

          // If the fill rate of the index is too low, we use this index level anyway
          if (nodeTypeData[i].indexCells/(1.0*nodeTypeData[i].cellXCount*nodeTypeData[i].cellYCount)<=
              parameter.GetAreaNodeIndexMinFillRate()) {
            progress.Warning(typeConfig->GetTypeInfo(i)->GetName()+" ("+std::to_string(i)+") is not well distributed");
            continue;
          }

          // If average fill size and max fill size for tile cells
          // is within limits, store it now.
          if (max<=parameter.GetAreaNodeIndexCellSizeMax() &&
              average<=parameter.GetAreaNodeIndexCellSizeAverage()) {
            continue;
          }

          // else, we remove it from the list and try again with an higher
          // level.
          currentNodeTypes.Remove(type);
        }

        // Now process all types for this limit, that are within the limits
        for (const auto& type : currentNodeTypes) {
          maxLevel=std::max(maxLevel,level);

          progress.Info("Type "+type->GetName()+"("+std::to_string(type->GetIndex())+"), "+
                        std::to_string(nodeTypeData[type->GetIndex()].indexCells)+" cells, "+
                        std::to_string(nodeTypeData[type->GetIndex()].indexEntries)+" objects");
        }

        remainingNodeTypes.Remove(currentNodeTypes);

        level++;
      }

      //
      // Writing index file
      //

      progress.SetAction("Generating 'areanode.idx'");

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AreaNodeIndex::AREA_NODE_IDX));

      uint32_t indexEntries=0;

      // Count number of types in index
      for (const auto& type : typeConfig->GetNodeTypes()) {
        if (nodeTypeData[type->GetIndex()].HasEntries()) {
          indexEntries++;
        }
      }

      writer.Write(indexEntries);

      // Store index data for each type
      for (const auto& type : typeConfig->GetNodeTypes()) {
        size_t i=type->GetIndex();

        if (nodeTypeData[i].HasEntries()) {
          FileOffset bitmapOffset=0;
          uint8_t dataOffsetBytes=0;

          writer.WriteNumber(type->GetNodeId());

          nodeTypeData[i].indexOffset=writer.GetPos();

          writer.WriteFileOffset(bitmapOffset);
          writer.Write(dataOffsetBytes);

          writer.WriteNumber(nodeTypeData[i].indexLevel);
          writer.WriteNumber(nodeTypeData[i].cellXStart);
          writer.WriteNumber(nodeTypeData[i].cellXEnd);
          writer.WriteNumber(nodeTypeData[i].cellYStart);
          writer.WriteNumber(nodeTypeData[i].cellYEnd);
        }
      }

      // Now store index bitmap for each type in increasing level order (why?)
      for (size_t l=0; l<=maxLevel; l++) {
        std::set<TypeInfoRef> indexTypes;
        uint32_t nodeCount;

        for (auto& type : typeConfig->GetNodeTypes()) {
          if (nodeTypeData[type->GetIndex()].HasEntries() &&
              nodeTypeData[type->GetIndex()].indexLevel==l) {
            indexTypes.insert(type);
          }
        }

        if (indexTypes.empty()) {
          continue;
        }

        progress.Info("Scanning nodes for index level "+std::to_string(l));

        std::vector<std::map<Pixel,std::list<FileOffset> > > typeCellOffsets;

        typeCellOffsets.resize(typeConfig->GetTypeCount());

        nodeScanner.GotoBegin();

        nodeScanner.Read(nodeCount);

        //
        // Collect all offsets
        //
        for (uint32_t n=1; n<=nodeCount; n++) {
          progress.SetProgress(n,nodeCount);

          FileOffset offset;
          Node node;

          offset=nodeScanner.GetPos();

          node.Read(*typeConfig,
                    nodeScanner);

          if (indexTypes.find(node.GetType())!=indexTypes.end()) {
            uint32_t xc=(uint32_t) floor((node.GetCoords().GetLon()+180.0)/cellDimension[l].width);
            uint32_t yc=(uint32_t) floor((node.GetCoords().GetLat()+90.0)/cellDimension[l].height);

            typeCellOffsets[node.GetType()->GetIndex()][Pixel(xc,yc)].push_back(offset);
          }
        }

        //
        // Write bitmap
        //
        for (const auto& type : indexTypes) {
          size_t indexEntries=0;
          size_t dataSize=0;
          char buffer[10];

          for (const auto& cell : typeCellOffsets[type->GetIndex()]) {
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

          progress.Info("Writing map for "+
                        type->GetName()+", "+
                        std::to_string(typeCellOffsets[type->GetIndex()].size())+" cells, "+
                        std::to_string(indexEntries)+" entries, "+
                        ByteSizeToString(1.0*dataOffsetBytes*nodeTypeData[type->GetIndex()].cellXCount*
                                         nodeTypeData[type->GetIndex()].cellYCount));

          FileOffset bitmapOffset;

          bitmapOffset=writer.GetPos();

          assert(nodeTypeData[type->GetIndex()].indexOffset!=0);

          writer.SetPos(nodeTypeData[type->GetIndex()].indexOffset);

          writer.WriteFileOffset(bitmapOffset);
          writer.Write(dataOffsetBytes);

          writer.SetPos(bitmapOffset);

          // Write the bitmap with offsets for each cell
          // We prefill with zero and only overwrite cells that have data
          // So zero means "no data for this cell"
          for (size_t i=0; i<nodeTypeData[type->GetIndex()].cellXCount*nodeTypeData[type->GetIndex()].cellYCount; i++) {
            FileOffset cellOffset=0;

            writer.WriteFileOffset(cellOffset,
                                   dataOffsetBytes);
          }

          FileOffset dataStartOffset;

          dataStartOffset=writer.GetPos();

          // Now write the list of offsets of objects for every cell with content
          for (const auto& cell : typeCellOffsets[type->GetIndex()]) {
            FileOffset bitmapCellOffset=bitmapOffset+
                                        ((cell.first.y-nodeTypeData[type->GetIndex()].cellYStart)*
                                         nodeTypeData[type->GetIndex()].cellXCount+
                                         cell.first.x-nodeTypeData[type->GetIndex()].cellXStart)*dataOffsetBytes;
            FileOffset previousOffset=0;
            FileOffset cellOffset;

            cellOffset=writer.GetPos();

            writer.SetPos(bitmapCellOffset);

            writer.WriteFileOffset(cellOffset-dataStartOffset+1,
                                   dataOffsetBytes);

            writer.SetPos(cellOffset);

            writer.WriteNumber((uint32_t) cell.second.size());

            for (auto offset : cell.second) {
              writer.WriteNumber((FileOffset) (offset-previousOffset));

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
}

