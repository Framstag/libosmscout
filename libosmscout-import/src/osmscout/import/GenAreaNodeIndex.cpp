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

#include <osmscout/Util.h>

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
    std::set<TypeId>      remainingNodeTypes;
    std::vector<TypeData> nodeTypeData;
    size_t                level;
    size_t                maxLevel=0;

    nodeTypeData.resize(typeConfig.GetTypes().size());

    if (!nodeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "nodes.dat"))) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    //
    // Scanning distribution
    //

    progress.SetAction("Scanning level distribution of node types");

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

      progress.Info("Scanning Level "+NumberToString(level)+" ("+NumberToString(remainingNodeTypes.size())+" types remaining)");

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

        // Count number of entries per current type and coordinate
        if (currentNodeTypes.find(node.GetType())!=currentNodeTypes.end()) {
          size_t xc=(size_t)floor((node.GetLon()+180.0)/cellWidth);
          size_t yc=(size_t)floor((node.GetLat()+90.0)/cellHeight);

          cellFillCount[node.GetType()][Coord(xc,yc)]++;
        }
      }

      // Check if cell fill for current type is in defined limits
      for (size_t i=0; i<typeConfig.GetTypes().size(); i++) {
        if (currentNodeTypes.find(i)!=currentNodeTypes.end()) {
          size_t entryCount=0;
          size_t max=0;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[i].begin();
               cell!=cellFillCount[i].end();
               ++cell) {
            entryCount+=cell->second;
            max=std::max(max,cell->second);
          }

          double average=entryCount*1.0/cellFillCount[i].size();

          if (!cellFillCount[i].empty() &&
              (max>parameter.GetAreaNodeIndexCellSizeMax() ||
               average>parameter.GetAreaNodeIndexCellSizeAverage())) {
            currentNodeTypes.erase(i);
          }
        }
      }

      for (std::set<TypeId>::const_iterator cnt=currentNodeTypes.begin();
           cnt!=currentNodeTypes.end();
           cnt++) {
        maxLevel=std::max(maxLevel,level);

        nodeTypeData[*cnt].indexLevel=level;
        nodeTypeData[*cnt].indexCells=cellFillCount[*cnt].size();
        nodeTypeData[*cnt].indexEntries=0;

        if (!cellFillCount[*cnt].empty()) {
          nodeTypeData[*cnt].cellXStart=cellFillCount[*cnt].begin()->first.x;
          nodeTypeData[*cnt].cellYStart=cellFillCount[*cnt].begin()->first.y;

          nodeTypeData[*cnt].cellXEnd=nodeTypeData[*cnt].cellXStart;
          nodeTypeData[*cnt].cellYEnd=nodeTypeData[*cnt].cellYStart;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount[*cnt].begin();
               cell!=cellFillCount[*cnt].end();
               ++cell) {
            nodeTypeData[*cnt].indexEntries+=cell->second;

            nodeTypeData[*cnt].cellXStart=std::min(nodeTypeData[*cnt].cellXStart,cell->first.x);
            nodeTypeData[*cnt].cellXEnd=std::max(nodeTypeData[*cnt].cellXEnd,cell->first.x);

            nodeTypeData[*cnt].cellYStart=std::min(nodeTypeData[*cnt].cellYStart,cell->first.y);
            nodeTypeData[*cnt].cellYEnd=std::max(nodeTypeData[*cnt].cellYEnd,cell->first.y);
          }
        }

        nodeTypeData[*cnt].cellXCount=nodeTypeData[*cnt].cellXEnd-nodeTypeData[*cnt].cellXStart+1;
        nodeTypeData[*cnt].cellYCount=nodeTypeData[*cnt].cellYEnd-nodeTypeData[*cnt].cellYStart+1;

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

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if (typeConfig.GetTypeInfo(i).CanBeNode() &&
          nodeTypeData[i].HasEntries()) {
        indexEntries++;
      }
    }

    writer.Write(indexEntries);

    for (size_t i=0; i<typeConfig.GetTypes().size(); i++)
    {
      if (typeConfig.GetTypeInfo(i).CanBeNode() &&
          nodeTypeData[i].HasEntries()) {
        FileOffset bitmapOffset=0;

        writer.WriteNumber(typeConfig.GetTypeInfo(i).GetId());

        writer.GetPos(nodeTypeData[i].indexOffset);

        writer.Write(bitmapOffset);

        writer.WriteNumber(nodeTypeData[i].indexLevel);
        writer.WriteNumber(nodeTypeData[i].cellXStart);
        writer.WriteNumber(nodeTypeData[i].cellXEnd);
        writer.WriteNumber(nodeTypeData[i].cellYStart);
        writer.WriteNumber(nodeTypeData[i].cellYEnd);
      }
    }

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
                      NumberToString(nodeTypeData[*type].cellXCount*nodeTypeData[*type].cellYCount)+
                      " map size");

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

        writer.Write(bitmapOffset);

        if (!writer.SetPos(bitmapOffset)) {
          progress.Error("Cannot go to type index start position in file");
          return false;
        }

        // Write the bitmap with offsets for each cell
        // We prefill with zero and only overwrite cells that have data
        // So zero means "no data for this cell"
        for (size_t i=0; i<nodeTypeData[*type].cellXCount*nodeTypeData[*type].cellYCount; i++) {
          FileOffset cellOffset=0;

          writer.Write(cellOffset);
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

          writer.Write(cellOffset);

          if (!writer.SetPos(cellOffset)) {
            progress.Error("Cannot go back to cell start position in file");
            return false;
          }

          writer.WriteNumber((uint32_t)cell->second.size());

          for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
               offset!=cell->second.end();
               ++offset) {
            writer.WriteNumber(*offset-previousOffset);

            previousOffset=*offset;
          }
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

