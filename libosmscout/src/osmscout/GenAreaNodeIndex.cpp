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

#include <osmscout/GenAreaNodeIndex.h>

#include <cmath>
#include <map>
#include <set>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Node.h>
#include <osmscout/Tiles.h>
#include <osmscout/Util.h>

namespace osmscout {

  static size_t maxLeafSize=10;

  struct NodeEntry
  {
    double     lat;
    double     lon;
    FileOffset offset;
  };

  struct NodeLeaf
  {
    size_t                 children[4];
    std::vector<NodeEntry> nodes;
    bool                   full;

    FileOffset             offset;

    NodeLeaf()
    {
      children[0]=0;
      children[1]=0;
      children[2]=0;
      children[3]=0;
      nodes.reserve(maxLeafSize);
      full=false;
      offset=0;
    }
  };

  static std::vector<double> cellWidth;
  static std::vector<double> cellHeight;

  static std::vector<double> cellWidthHalf;
  static std::vector<double> cellHeightHalf;

  void AddEntry(size_t leaf,
                std::vector<NodeLeaf>& leafs,
                double lat,
                double lon,
                FileOffset offset,
                size_t level)
  {
    if (leafs[leaf].full) {
      size_t index;

      if (lon-floor(lon/cellWidth[level])*cellWidth[level]<cellWidth[level+1]) {
        if (lat-floor(lat/cellHeight[level])*cellHeight[level]<cellHeight[level+1]) {
          index=2;
        }
        else {
          index=0;
        }
      }
      else {
        if (lat-floor(lat/cellHeight[level])*cellHeight[level]<cellHeight[level+1]) {
          index=3;
        }
        else {
          index=1;
        }
      }

      if (leafs[leaf].children[index]==0) {
        NodeLeaf l;

        leafs[leaf].children[index]=leafs.size();

        leafs.push_back(l);
      }

      AddEntry(leafs[leaf].children[index],leafs,lat,lon,offset,level+1);
    }
    else if (level==cellWidth.size()-1 ||
             leafs[leaf].nodes.size()+1<=maxLeafSize) {
      NodeEntry entry;

      entry.lat=lat;
      entry.lon=lon;
      entry.offset=offset;

      leafs[leaf].nodes.push_back(entry);
    }
    else {
      // Leaf is full, redelegate all entries in this leaf to its children
      // by reinserting them to the current leaf after setting the leaf to 'full'.

      // Make a copy, since AddEntry does does modifications on leafs array
      std::vector<NodeEntry> entries=leafs[leaf].nodes;

      leafs[leaf].nodes.clear();
      leafs[leaf].full=true;

      for (std::vector<NodeEntry>::const_iterator entry=entries.begin();
           entry!=entries.end();
           ++entry) {
        AddEntry(leaf,
                 leafs,
                 entry->lat,
                 entry->lon,
                 entry->offset,
                 level);
      }

      AddEntry(leaf,leafs,lat,lon,offset,level);
    }
  }

  std::string AreaNodeIndexGenerator::GetDescription() const
  {
    return "Generate 'areanode.idx'";
  }

  bool AreaNodeIndexGenerator::Import(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig)
  {
    //
    // Analysing nodes regarding draw type and matching tiles.
    //

    progress.SetAction("Analysing distribution");

    FileScanner                         scanner;
    uint32_t                            nodeCount=0;
    uint32_t                            drawTypeCount=0;
    uint32_t                            leafCount=0;
    std::vector<std::vector<NodeLeaf> > leafs;

    cellWidth.resize(18);
    cellHeight.resize(18);

    cellWidthHalf.resize(18);
    cellHeightHalf.resize(18);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360.0/pow(2.0,(int)i);
      cellWidthHalf[i]=cellWidth[i]/2;
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180.0/pow(2.0,(int)i);
      cellHeightHalf[i]=cellHeight[i]/2;
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "nodes.dat"))) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    if (!scanner.Read(nodeCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t n=1; n<=nodeCount; n++) {
      progress.SetProgress(n,nodeCount);

      FileOffset offset;
      Node       node;

      if (!scanner.GetPos(offset)){
        progress.Error("Error while reading data offset");
        return false;
      }

      if (!node.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(nodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (node.GetType()>=leafs.size()) {
        leafs.resize(node.GetType()+1);
      }
      if (leafs[node.GetType()].size()==0){
        leafs[node.GetType()].resize(1);
      }

      AddEntry(0,
               leafs[node.GetType()],
               node.GetLat()+90,
               node.GetLon()+180,
               offset,
               0);
    }

    scanner.Close();

    progress.Info(std::string("Nodes scanned: ")+NumberToString(nodeCount));

    drawTypeCount=0;
    for (size_t i=0; i<leafs.size(); i++) {
      if (leafs[i].size()>0) {
        drawTypeCount++;
        leafCount+=leafs[i].size();
      }
    }

    progress.Info(NumberToString(drawTypeCount)+" node types");
    progress.Info(NumberToString(leafCount)+" leafs");

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areanode.idx'");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areanode.idx"))) {
      progress.Error("Cannot create 'nodes.dat'");
      return false;
    }

    writer.WriteNumber(drawTypeCount);    // Number of entries
    writer.WriteNumber(leafs.size()-1);   // Type id of highest entry
    writer.WriteNumber(cellWidth.size()); // maximum level

    FileOffset tableOffset;

    if (!writer.GetPos(tableOffset)) {
      progress.Error("Error while getting current file position");
      return false;
    }

    // Index lookup table
    for (TypeId i=0; i<leafs.size(); i++) {
      if (leafs[i].size()>0) {
        writer.Write(i); // Type
        writer.Write(0); // Offset of top index entry
      }
    }

    // Index pages
    for (TypeId i=0; i<leafs.size(); i++) {
      if (leafs[i].size()>0) {
        for (std::vector<NodeLeaf>::reverse_iterator leaf=leafs[i].rbegin();
             leaf!=leafs[i].rend();
             ++leaf) {
          if (!writer.GetPos(leaf->offset)) {
            return false;
          }

          // Resolve offsets of children
          // TODO: Write offsets relative to our offset
          if (leaf->children[0]!=0) {
            writer.WriteNumber(leafs[i][leaf->children[0]].offset);
          }
          else {
            writer.WriteNumber(0);
          }

          if (leaf->children[1]!=0) {
            writer.WriteNumber(leafs[i][leaf->children[1]].offset);
          }
          else {
            writer.WriteNumber(0);
          }

          if (leaf->children[2]!=0) {
            writer.WriteNumber(leafs[i][leaf->children[2]].offset);
          }
          else {
            writer.WriteNumber(0);
          }

          if (leaf->children[3]!=0) {
            writer.WriteNumber(leafs[i][leaf->children[3]].offset);
          }
          else {
            writer.WriteNumber(0);
          }

          writer.WriteNumber(leaf->nodes.size());

          for (std::vector<NodeEntry>::const_iterator entry=leaf->nodes.begin();
               entry!=leaf->nodes.end();
               ++entry) {
            writer.WriteNumber(entry->offset);
          }
        }
      }
    }

    if (!writer.SetPos(tableOffset)) {
      progress.Error("Error while setting current file position");
      return false;
    }

    for (TypeId i=0; i<leafs.size(); i++) {
      if (leafs[i].size()>0) {
        //std::cout << i << " " << leafs[i][0].offset << " " << leafs[i][leafs[i][0].children[0]].offset << " " << leafs[i][leafs[i][0].children[1]].offset << " " << leafs[i][leafs[i][0].children[2]].offset << " " << leafs[i][leafs[i][0].children[3]].offset << std::endl;
        writer.Write(i);
        writer.Write(leafs[i][0].offset);
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

