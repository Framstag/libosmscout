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

#include <cassert>
#include <vector>

#include <osmscout/Node.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

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

  std::string AreaNodeIndexGenerator::GetDescription() const
  {
    return "Generate 'areanode.idx'";
  }

  bool AreaNodeIndexGenerator::Import(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig)
  {
    FileScanner           nodeScanner;
    FileWriter            writer;
    std::set<TypeId>      remainingNodeTypes; //! Set of types we still must process
    std::vector<TypeData> nodeTypeData;
    size_t                level;
    size_t                maxLevel=0;

    nodeTypeData.resize(typeConfig.GetTypes().size());

    if (!nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "nodes.dat"),
                          FileScanner::SequentialScan,
                          true)) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    //
    // Scanning distribution
    //

    progress.SetAction("Scanning level distribution of node types");

    // Initially we must process all types that represents nodes and that should
    // not be ignored
    for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
      if (typeConfig.GetTypeInfo(i).CanBeNode() &&
          !typeConfig.GetTypeInfo(i).GetIgnore()) {
        remainingNodeTypes.insert(i);
      }
    }

    level=parameter.GetAreaNodeMinMag();
    while (!remainingNodeTypes.empty()) {
      uint32_t         nodeCount=0;
      std::set<TypeId> currentNodeTypes(remainingNodeTypes);
      double           cellWidth=360.0/pow(2.0,(int)level);
      double           cellHeight=180.0/pow(2.0,(int)level);
      std::vector<std::map<Coord,size_t> > cellFillCount;

      cellFillCount.resize(typeConfig.GetTypes().size());

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingNodeTypes.size())+" types still to process)");

      nodeScanner.GotoBegin();

      if (!nodeScanner.Read(nodeCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        FileOffset offset;
        Node       node;

        nodeScanner.GetPos(offset);

        if (!node.Read(nodeScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(n)+" of "+
                         NumberToString(nodeCount)+
                         " in file '"+
                         nodeScanner.GetFilename()+"'");
          return false;
        }

        // If we still need to handle this type,
        // count number of entries per type and tile cell
        if (currentNodeTypes.find(node.GetType())!=currentNodeTypes.end()) {
          size_t xc=(size_t)floor((node.GetLon()+180.0)/cellWidth);
          size_t yc=(size_t)floor((node.GetLat()+90.0)/cellHeight);

          cellFillCount[node.GetType()][Coord(xc,yc)]++;
        }
      }

      // Check statistics for each type
      // If statistics are within goal limits, use this level
      // for this type (else try again with the next higher level)
      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentNodeTypes.find(i)!=currentNodeTypes.end()) {
          size_t entryCount=0;
          size_t max=0;

          nodeTypeData[i].indexLevel=level;
          nodeTypeData[i].indexCells=cellFillCount[i].size();
          nodeTypeData[i].indexEntries=0;

          if (!cellFillCount[i].empty()) {
            nodeTypeData[i].cellXStart=cellFillCount[i].begin()->first.x;
            nodeTypeData[i].cellYStart=cellFillCount[i].begin()->first.y;

            nodeTypeData[i].cellXEnd=nodeTypeData[i].cellXStart;
            nodeTypeData[i].cellYEnd=nodeTypeData[i].cellYStart;

            for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[i].begin();
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
          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[i].begin();
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
            progress.Warning(typeConfig.GetTypeInfo(i).GetName()+" ("+NumberToString(i)+") is not well distributed");
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
          currentNodeTypes.erase(i);
        }
      }

      // Now process all types for this limit, that are within the limits
      for (std::set<TypeId>::const_iterator cnt=currentNodeTypes.begin();
           cnt!=currentNodeTypes.end();
           cnt++) {
        maxLevel=std::max(maxLevel,level);

        progress.Info("Type "+typeConfig.GetTypeInfo(*cnt).GetName()+"(" + NumberToString(*cnt)+"), "+NumberToString(nodeTypeData[*cnt].indexCells)+" cells, "+NumberToString(nodeTypeData[*cnt].indexEntries)+" objects");

        remainingNodeTypes.erase(*cnt);
      }

      level++;
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areanode.idx'");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areanode.idx"))) {
      progress.Error("Cannot create 'areanode.idx'");
      return false;
    }

    uint32_t indexEntries=0;

    // Count number of types in index
    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if (typeConfig.GetTypeInfo(i).CanBeNode() &&
          nodeTypeData[i].HasEntries()) {
        indexEntries++;
      }
    }

    writer.Write(indexEntries);

    // Store index data for each type
    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if (typeConfig.GetTypeInfo(i).CanBeNode() &&
          nodeTypeData[i].HasEntries()) {
        FileOffset bitmapOffset=0;

        writer.WriteNumber(typeConfig.GetTypeInfo(i).GetId());

        writer.GetPos(nodeTypeData[i].indexOffset);

        writer.WriteFileOffset(bitmapOffset);

        writer.WriteNumber(nodeTypeData[i].indexLevel);
        writer.WriteNumber(nodeTypeData[i].cellXStart);
        writer.WriteNumber(nodeTypeData[i].cellXEnd);
        writer.WriteNumber(nodeTypeData[i].cellYStart);
        writer.WriteNumber(nodeTypeData[i].cellYEnd);
      }
    }

    // Now store index bitmap for each type in increasing level order (why?)
    for (size_t l=0; l<=maxLevel; l++) {
      std::set<TypeId> indexTypes;
      uint32_t         nodeCount;
      double           cellWidth=360.0/pow(2.0,(int)l);
      double           cellHeight=180.0/pow(2.0,(int)l);

      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (typeConfig.GetTypeInfo(i).CanBeNode() &&
            nodeTypeData[i].HasEntries() &&
            nodeTypeData[i].indexLevel==l) {
          indexTypes.insert(i);
        }
      }

      if (indexTypes.empty()) {
        continue;
      }

      progress.Info("Scanning nodes for index level "+NumberToString(l));

      std::vector<std::map<Coord,std::list<FileOffset> > > typeCellOffsets;

      typeCellOffsets.resize(typeConfig.GetTypes().size());

      nodeScanner.GotoBegin();

      if (!nodeScanner.Read(nodeCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      //
      // Collect all offsets
      //
      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        FileOffset offset;
        Node       node;

        nodeScanner.GetPos(offset);

        if (!node.Read(nodeScanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(n)+" of "+
                         NumberToString(nodeCount)+
                         " in file '"+
                         nodeScanner.GetFilename()+"'");
          return false;
        }

        if (indexTypes.find(node.GetType())!=indexTypes.end()) {
          size_t xc=(size_t)floor((node.GetLon()+180.0)/cellWidth);
          size_t yc=(size_t)floor((node.GetLat()+90.0)/cellHeight);

          typeCellOffsets[node.GetType()][Coord(xc,yc)].push_back(offset);
        }
      }

      //
      // Write bitmap
      //
      for (std::set<TypeId>::const_iterator type=indexTypes.begin();
           type!=indexTypes.end();
           ++type) {
        size_t   indexEntries=0;
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          indexEntries+=cell->second.size();
        }

        progress.Info("Writing bitmap for "+
                      typeConfig.GetTypeInfo(*type).GetName()+" ("+NumberToString(*type)+"), "+
                      NumberToString(typeCellOffsets[*type].size())+" cells, "+
                      NumberToString(indexEntries)+" entries, "+
                      ByteSizeToString(1.0*sizeof(FileOffset)*nodeTypeData[*type].cellXCount*nodeTypeData[*type].cellYCount)+" bitmap");

        FileOffset bitmapOffset;

        if (!writer.GetPos(bitmapOffset)) {
          progress.Error("Cannot get type index start position in file");
          return false;
        }

        assert(nodeTypeData[*type].indexOffset!=0);

        if (!writer.SetPos(nodeTypeData[*type].indexOffset)) {
          progress.Error("Cannot go to type index offset in file");
          return false;
        }

        writer.WriteFileOffset(bitmapOffset);

        if (!writer.SetPos(bitmapOffset)) {
          progress.Error("Cannot go to type index start position in file");
          return false;
        }

        // Write the bitmap with offsets for each cell
        // We prefill with zero and only overwrite cells that have data
        // So zero means "no data for this cell"
        for (size_t i=0; i<nodeTypeData[*type].cellXCount*nodeTypeData[*type].cellYCount; i++) {
          FileOffset cellOffset=0;

          writer.WriteFileOffset(cellOffset);
        }

        // Now write the list of offsets of objects for every cell with content
        for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=typeCellOffsets[*type].begin();
             cell!=typeCellOffsets[*type].end();
             ++cell) {
          FileOffset bitmapCellOffset=bitmapOffset+
                                      ((cell->first.y-nodeTypeData[*type].cellYStart)*nodeTypeData[*type].cellXCount+
                                       cell->first.x-nodeTypeData[*type].cellXStart)*sizeof(FileOffset);
          FileOffset previousOffset=0;
          FileOffset cellOffset;

          if (!writer.GetPos(cellOffset)) {
            progress.Error("Cannot get cell start position in file");
            return false;
          }

          if (!writer.SetPos(bitmapCellOffset)) {
            progress.Error("Cannot go to cell start position in file");
            return false;
          }

          writer.WriteFileOffset(cellOffset);

          if (!writer.SetPos(cellOffset)) {
            progress.Error("Cannot go back to cell start position in file");
            return false;
          }

          writer.WriteNumber((uint32_t)cell->second.size());

          for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
               offset!=cell->second.end();
               ++offset) {
            writer.WriteNumber((FileOffset)(*offset-previousOffset));

            previousOffset=*offset;
          }
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

