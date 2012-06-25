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

#include <osmscout/import/GenOptimizeLowZoom.h>

#include <cassert>

#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/private/Math.h>

namespace osmscout
{
  OptimizeLowZoomGenerator::TypeData::TypeData()
  : indexLevel(0),
    indexCells(0),
    indexEntries(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    cellXCount(0),
    cellYCount(0),
    bitmapOffset(0)
  {
    // no code
  }

  std::string OptimizeLowZoomGenerator::GetDescription() const
  {
    return "Generate 'optimize.dat'";
  }

  void OptimizeLowZoomGenerator::GetTypesToOptimize(const TypeConfig& typeConfig,
                                                    std::set<TypeId>& types)
  {
    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
        type!=typeConfig.GetTypes().end();
        type++) {
      if (type->GetOptimizeLowZoom()) {
        types.insert(type->GetId());
      }
    }
  }

  void OptimizeLowZoomGenerator::WriteHeader(FileWriter& writer,
                                             const std::set<TypeId>& types,
                                             const std::vector<TypeData>& typesData,
                                             size_t optimizeMaxMap,
                                             std::map<TypeId,FileOffset>& typeOffsetMap)
  {
    writer.SetPos(0);

    writer.Write((uint32_t)optimizeMaxMap);
    writer.Write((uint32_t)types.size());

    for (std::set<TypeId>::const_iterator type=types.begin();
        type!=types.end();
        type++) {
      writer.Write(*type);
      writer.Write(typesData[*type].indexLevel);
      writer.Write(typesData[*type].cellXStart);
      writer.Write(typesData[*type].cellXEnd);
      writer.Write(typesData[*type].cellYStart);
      writer.Write(typesData[*type].cellYEnd);
      writer.WriteFileOffset(typesData[*type].bitmapOffset);
    }
  }

  bool OptimizeLowZoomGenerator::GetWaysToOptimize(Progress& progress,
                                                   FileScanner& scanner,
                                                   TypeId type,
                                                   std::list<WayRef>& ways)
  {
    size_t   nodes=0;
    uint32_t wayCount=0;

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      WayRef way=new Way();

      progress.SetProgress(w,wayCount);

      if (!way->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
            NumberToString(w)+" of "+
            NumberToString(wayCount)+
            " in file '"+
            scanner.GetFilename()+"'");
        return false;
      }

      if (!way->IsArea() &&
          way->GetType()==type &&
          way->nodes.size()>=2) {
        ways.push_back(way);
        nodes+=way->nodes.size();
      }
    }

    progress.Info(NumberToString(ways.size())+ " ways, " + NumberToString(nodes)+ " nodes");

    return true;
  }

  void OptimizeLowZoomGenerator::MergeWays(const std::list<WayRef>& ways,
                                           std::list<WayRef>& newWays)
  {
    std::map<Id, std::list<WayRef > > waysByJoin;
    std::set<Id>                      usedWays;

    for (std::list<WayRef>::const_iterator way=ways.begin();
        way!=ways.end();
        way++) {
      waysByJoin[(*way)->nodes.front().GetId()].push_back(*way);
      waysByJoin[(*way)->nodes.back().GetId()].push_back(*way);
    }

    for (std::map<Id, std::list<WayRef> >::iterator entry=waysByJoin.begin();
        entry!=waysByJoin.end();
        entry++) {
      while (!entry->second.empty()) {

        WayRef way=entry->second.front();

        entry->second.pop_front();

        if (usedWays.find(way->GetId())!=usedWays.end()) {
          continue;
        }

        usedWays.insert(way->GetId());

        while (true) {
          std::map<Id, std::list<WayRef> >::iterator match;

          match=waysByJoin.find(way->nodes.front().GetId());
          if (match!=waysByJoin.end())
          {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetId())!=usedWays.end() ||
                    way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetId())!=usedWays.end() ||
                      way->GetRefName()!=(*stillOtherWay)->GetRefName())) {
                stillOtherWay++;
              }

              // If we have at least three ways with the same joining node
              // we do not merge because we have a multi-way junctions
              if (stillOtherWay!=match->second.end()) {
                otherWay=match->second.end();
              }
            }

            if (otherWay!=match->second.end()) {
              std::vector<Point> newNodes;

              newNodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->nodes.front().GetId()==(*otherWay)->nodes.front().GetId()) {
                for (size_t i=(*otherWay)->nodes.size()-1; i>0; i--) {
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=0; i<way->nodes.size(); i++) {
                  newNodes.push_back(way->nodes[i]);
                }

                way->nodes=newNodes;
              }
              else {
                for (size_t i=0; i<(*otherWay)->nodes.size(); i++) {
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=1; i<way->nodes.size(); i++) {
                  newNodes.push_back(way->nodes[i]);
                }

                way->nodes=newNodes;
              }

              usedWays.insert((*otherWay)->GetId());
              match->second.erase(otherWay);

              continue;
            }
          }

          match=waysByJoin.find(way->nodes.back().GetId());
          if (match!=waysByJoin.end())
          {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetId())!=usedWays.end() ||
                     way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetId())!=usedWays.end() ||
                      way->GetRefName()!=(*stillOtherWay)->GetRefName())) {
                stillOtherWay++;
              }

              // If we have at least three ways with the same joining node
              // we do not merge because we have a multi-way junctions
              if (stillOtherWay!=match->second.end()) {
                otherWay=match->second.end();
              }
            }

            if (otherWay!=match->second.end()) {
              way->nodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->nodes.back().GetId()==(*otherWay)->nodes.front().GetId()) {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  way->nodes.push_back((*otherWay)->nodes[i]);
                }
              }
              else {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  size_t idx=(*otherWay)->nodes.size()-1-i;

                  way->nodes.push_back((*otherWay)->nodes[idx]);
                }
              }

              usedWays.insert((*otherWay)->GetId());
              match->second.erase(otherWay);

              continue;
            }
          }

          break;
        }

        newWays.push_back(*way);
      }
    }
  }

  void OptimizeLowZoomGenerator::GetIndexLevel(const ImportParameter& parameter,
                                               Progress& progress,
                                               const std::list<WayRef>& newWays,
                                               TypeData& typeData)
  {
    size_t level=parameter.GetOptimizationMinMag();

    while (true) {
      double                 cellWidth=360.0/pow(2.0,(int)level);
      double                 cellHeight=180.0/pow(2.0,(int)level);
      std::map<Coord,size_t> cellFillCount;

      for (std::list<WayRef>::const_iterator w=newWays.begin();
          w!=newWays.end();
          ++w) {
        WayRef way=*w;
        // Count number of entries per current type and coordinate
        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        way->GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormated coordinate space (everything is >=0)
        //
        size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
        size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
        size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
        size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

        for (size_t y=minyc; y<=maxyc; y++) {
          for (size_t x=minxc; x<=maxxc; x++) {
            cellFillCount[Coord(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      size_t entryCount=0;
      size_t max=0;

      for (std::map<Coord,size_t>::const_iterator cell=cellFillCount.begin();
           cell!=cellFillCount.end();
           ++cell) {
        entryCount+=cell->second;
        max=std::max(max,cell->second);
      }

      double average=entryCount*1.0/cellFillCount.size();

      if (!(max>parameter.GetOptimizationCellSizeMax() ||
           average>parameter.GetOptimizationCellSizeAverage())) {
        typeData.indexLevel=level;
        typeData.indexCells=cellFillCount.size();
        typeData.indexEntries=0;

        if (!cellFillCount.empty()) {
          typeData.cellXStart=cellFillCount.begin()->first.x;
          typeData.cellYStart=cellFillCount.begin()->first.y;

          typeData.cellXEnd=typeData.cellXStart;
          typeData.cellYEnd=typeData.cellYStart;

          for (std::map<Coord,size_t>::const_iterator cell=cellFillCount.begin();
               cell!=cellFillCount.end();
               ++cell) {
            typeData.indexEntries+=cell->second;

            typeData.cellXStart=std::min(typeData.cellXStart,cell->first.x);
            typeData.cellXEnd=std::max(typeData.cellXEnd,cell->first.x);

            typeData.cellYStart=std::min(typeData.cellYStart,cell->first.y);
            typeData.cellYEnd=std::max(typeData.cellYEnd,cell->first.y);
          }
        }

        typeData.cellXCount=typeData.cellXEnd-typeData.cellXStart+1;
        typeData.cellYCount=typeData.cellYEnd-typeData.cellYStart+1;

        return;
      }

      level++;
    }
  }

  bool OptimizeLowZoomGenerator::WriteOptimizedWays(Progress& progress,
                                                   FileWriter& writer,
                                                   const std::list<WayRef>& ways,
                                                   IdFileOffsetMap& offsets,
                                                   size_t width,
                                                   size_t height,
                                                   double magnification,
                                                   TransPolygon::OptimizeMethod optimizeWayMethod)
  {
    size_t             wayCount=0;
    size_t             nodeCount=0;
    MercatorProjection projection;

    projection.Set(0,0,magnification,width,height);

    for (std::list<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        w++) {
      WayRef             way(*w);
      TransPolygon       polygon;
      std::vector<Point> newNodes;
      double             xmin;
      double             xmax;
      double             ymin;
      double             ymax;

      polygon.TransformWay(projection,optimizeWayMethod,way->nodes, 1.0);

      polygon.GetBoundingBox(xmin,ymin,xmax,ymax);

      if (xmax-xmin<=1.0 && ymax-ymin<=1.0) {
        continue;
      }

      newNodes.reserve(polygon.GetLength());

      for (size_t i=polygon.GetStart(); i<=polygon.GetEnd(); i++) {
        if (polygon.points[i].draw) {
          newNodes.push_back(way->nodes[i]);
        }
      }

      FileOffset offset;

      writer.GetPos(offset);

      offsets[way->GetId()]=offset;

      way->nodes=newNodes;

      if (!way->Write(writer)) {
        return false;
      }

      wayCount++;
      nodeCount+=way->nodes.size();
    }

    progress.Info(NumberToString(wayCount)+ " ways, "+NumberToString(nodeCount)+ " nodes");

    return true;
  }

  bool OptimizeLowZoomGenerator::WriteBitmap(Progress& progress,
                                             FileWriter& writer,
                                             const TypeInfo& type,
                                             const std::list<WayRef>& ways,
                                             const IdFileOffsetMap& offsets,
                                             TypeData& data)
  {
    // We do not write a bitmap, if there is not data to map
    if (ways.empty()) {
      return true;
    }

    double                                 cellWidth=360.0/pow(2.0,(int)data.indexLevel);
    double                                 cellHeight=180.0/pow(2.0,(int)data.indexLevel);
    std::map<Coord,std::list<FileOffset> > cellOffsets;

    for (std::list<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        w++) {
      WayRef                          way(*w);
      double                          minLon;
      double                          maxLon;
      double                          minLat;
      double                          maxLat;
      IdFileOffsetMap::const_iterator offset=offsets.find(way->GetId());

      if (offset==offsets.end()) {
        continue;
      }

      way->GetBoundingBox(minLon,maxLon,minLat,maxLat);

      //
      // Calculate minimum and maximum tile ids that are covered
      // by the way
      // Renormated coordinate space (everything is >=0)
      //
      size_t minxc=(size_t)floor((minLon+180.0)/cellWidth);
      size_t maxxc=(size_t)floor((maxLon+180.0)/cellWidth);
      size_t minyc=(size_t)floor((minLat+90.0)/cellHeight);
      size_t maxyc=(size_t)floor((maxLat+90.0)/cellHeight);

      for (size_t y=minyc; y<=maxyc; y++) {
        for (size_t x=minxc; x<=maxxc; x++) {
          cellOffsets[Coord(x,y)].push_back(offset->second);
        }
      }
    }

    size_t indexEntries=0;

    for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      indexEntries+=cell->second.size();
    }

    progress.Info("Writing bitmap for "+
                  type.GetName()+" ("+NumberToString(type.GetId())+"), "+
                  "level "+NumberToString(data.indexLevel)+", "+
                  NumberToString(cellOffsets.size())+" cells, "+
                  NumberToString(indexEntries)+" entries");

    if (!writer.GetPos(data.bitmapOffset)) {
      progress.Error("Cannot get type index start position in file");
      return false;
    }

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<data.cellXCount*data.cellYCount; i++) {
      FileOffset cellOffset=0;

      writer.WriteFileOffset(cellOffset);
    }

    // Now write the list of offsets of objects for every cell with content
    for (std::map<Coord,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      FileOffset bitmapCellOffset=data.bitmapOffset+
                                  ((cell->first.y-data.cellYStart)*data.cellXCount+
                                   cell->first.x-data.cellXStart)*sizeof(FileOffset);
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

    return true;
  }

  bool OptimizeLowZoomGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    std::set<TypeId>            types;         // Types we optimize
    FileScanner                 wayScanner;
    FileWriter                  writer;
    std::map<TypeId,FileOffset> dataOffsets;   // FileOffset where we store the file offset of the type data
    double                      magnification; // Magnification, we optimize for
    std::vector<TypeData>       typesData;

    typesData.resize(typeConfig.GetTypes().size());

    magnification=pow(2.0,(int)parameter.GetOptimizationMaxMag());

    progress.SetAction("Getting types to optimize");

    GetTypesToOptimize(typeConfig,types);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "optimized.dat"))) {
      progress.Error("Cannot create 'relations.dat'");
      return false;
    }

    //
    // Write header
    //

    WriteHeader(writer,
                types,
                typesData,
                parameter.GetOptimizationMaxMag(),
                dataOffsets);

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
        "ways.dat"))) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    for (std::set<TypeId>::const_iterator type=types.begin();
         type!=types.end();
         type++) {
      std::list<WayRef> ways;
      std::list<WayRef> newWays;
      IdFileOffsetMap   offsets;

      //
      // Load type data
      //

      progress.SetAction("Scanning ways for type '" + typeConfig.GetTypeInfo(*type).GetName() + "' to gather data to optimize");


      if (!GetWaysToOptimize(progress,
                             wayScanner,
                             *type,
                             ways))
      {
        return false;
      }

      //
      // Join ways
      //

      progress.SetAction("Merging ways");

      MergeWays(ways,newWays);

      ways.clear();

      if (newWays.empty()) {
        continue;
      }

      GetIndexLevel(parameter,
                    progress,
                    newWays,
                    typesData[*type]);

      //
      // Transform/Optimize the way and store it
      //

      // TODO: Wee need to make import parameters for the width and the height

      if (!WriteOptimizedWays(progress,
                              writer,
                              newWays,
                              offsets,
                              800,640,
                              magnification,
                              parameter.GetOptimizationWayMethod())) {
        return false;
      }

      if (!WriteBitmap(progress,
                       writer,
                       typeConfig.GetTypeInfo(*type),
                       newWays,
                       offsets,
                       typesData[*type])) {
        return false;
      }
    }

    WriteHeader(writer,
                types,
                typesData,
                parameter.GetOptimizationMaxMag(),
                dataOffsets);

    return !wayScanner.HasError() && wayScanner.Close();
  }
}

