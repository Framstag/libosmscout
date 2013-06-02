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

#include <osmscout/Pixel.h>
#include <osmscout/Way.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <iostream>
namespace osmscout
{
  OptimizeLowZoomGenerator::TypeData::TypeData()
  : type(0),
    optLevel(0),
    indexLevel(0),
    cellXStart(0),
    cellXEnd(0),
    cellYStart(0),
    cellYEnd(0),
    bitmapOffset(0),
    dataOffsetBytes(0),
    cellXCount(0),
    cellYCount(0),
    indexCells(0),
    indexEntries(0)
  {
    // no code
  }

  std::string OptimizeLowZoomGenerator::GetDescription() const
  {
    return "Generate 'optimized.dat'";
  }

  void OptimizeLowZoomGenerator::GetWayTypesToOptimize(const TypeConfig& typeConfig,
                                                       std::set<TypeId>& types)
  {
    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
        type!=typeConfig.GetTypes().end();
        type++) {
      if (type->GetOptimizeLowZoom() &&
          type->CanBeWay()) {
        types.insert(type->GetId());
      }
    }
  }

  void OptimizeLowZoomGenerator::GetAreaTypesToOptimize(const TypeConfig& typeConfig,
                                                       std::set<TypeId>& types)
  {
    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
        type!=typeConfig.GetTypes().end();
        type++) {
      if (type->GetOptimizeLowZoom() &&
          type->CanBeArea()) {
        types.insert(type->GetId());
      }
    }
  }

  bool OptimizeLowZoomGenerator::WriteTypeData(FileWriter& writer,
                                               const TypeData& data)
  {
    writer.Write(data.type);
    writer.Write(data.optLevel);
    writer.Write(data.indexLevel);
    writer.Write(data.cellXStart);
    writer.Write(data.cellXEnd);
    writer.Write(data.cellYStart);
    writer.Write(data.cellYEnd);

    writer.WriteFileOffset(data.bitmapOffset);
    writer.Write(data.dataOffsetBytes);

    return !writer.HasError();
  }

  bool OptimizeLowZoomGenerator::WriteHeader(FileWriter& writer,
                                             const std::list<TypeData>& areaTypesData,
                                             const std::set<TypeId>& wayTypes,
                                             const std::vector<TypeData>& wayTypesData,
                                             uint32_t optimizeMaxMap)
  {
    writer.Write(optimizeMaxMap);
    writer.Write((uint32_t)areaTypesData.size());
    writer.Write((uint32_t)wayTypes.size());

    for (std::list<TypeData>::const_iterator typeData=areaTypesData.begin();
        typeData!=areaTypesData.end();
        ++typeData) {
      if (!WriteTypeData(writer,
                         *typeData)) {
        return false;
      }
    }

    for (std::set<TypeId>::const_iterator type=wayTypes.begin();
        type!=wayTypes.end();
        type++) {
      if (!WriteTypeData(writer,
                         wayTypesData[*type])) {
        return false;
      }
    }

    return true;
  }

  bool OptimizeLowZoomGenerator::GetAreas(const ImportParameter& parameter,
                                          Progress& progress,
                                          FileScanner& scanner,
                                          std::set<TypeId>& types,
                                          std::vector<std::list<AreaRef> >& areas)
  {
    uint32_t         areaCount=0;
    size_t           collectedAreasCount=0;
    std::set<TypeId> currentTypes(types);

    progress.SetAction("Collecting area data to optimize");

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t a=1; a<=areaCount; a++) {
      AreaRef area=new Area();

      progress.SetProgress(a,areaCount);

      if (!area->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
            NumberToString(a)+" of "+
            NumberToString(areaCount)+
            " in file '"+
            scanner.GetFilename()+"'");
        return false;
      }

      if (currentTypes.find(area->GetType())!=currentTypes.end()) {
        areas[area->GetType()].push_back(area);

        collectedAreasCount++;

        while (collectedAreasCount>parameter.GetOptimizationMaxWayCount() &&
               currentTypes.size()>1) {
          size_t victimType=areas.size();

          for (size_t i=0; i<areas.size(); i++) {
            if (areas[i].size()>0 &&
                (victimType>=areas.size() ||
                 areas[i].size()<areas[victimType].size())) {
              victimType=i;
            }
          }

          if (victimType<areas.size()) {
            collectedAreasCount-=areas[victimType].size();
            areas[victimType].clear();
            currentTypes.erase(victimType);
          }
        }
      }
    }

    for (std::set<TypeId>::const_iterator type=currentTypes.begin();
         type!=currentTypes.end();
         ++type) {
      types.erase(*type);
    }

    progress.Info("Collected "+NumberToString(collectedAreasCount)+" areas for "+NumberToString(currentTypes.size())+" types");

    return true;
  }

  void OptimizeLowZoomGenerator::OptimizeAreas(Progress& progress,
                                               const TypeConfig& typeConfig,
                                               TypeId type,
                                               const std::list<AreaRef>& areas,
                                               std::list<AreaRef>& optimizedAreas,
                                               size_t width,
                                               size_t height,
                                               const Magnification& magnification,
                                               TransPolygon::OptimizeMethod optimizeWayMethod)
  {
    /*
    size_t origAreas=areas.size();
    size_t origRoles=0;
    size_t finalRoles=0;
    size_t origNodes=0;
    size_t finalNodes=0;*/

    MercatorProjection projection;

    projection.Set(0,0,magnification,width,height);

    for (std::list<AreaRef>::const_iterator a=areas.begin();
         a!=areas.end();
         ++a) {
      AreaRef                 area(*a);
      TransPolygon            polygon;
      std::vector<Area::Role> newRoles;
      double                  xmin;
      double                  xmax;
      double                  ymin;
      double                  ymax;

      /*
      origRoles+=area->roles.size();

      newRoles.reserve(area->roles.size());

      for (size_t r=0; r<area->roles.size(); r++) {
        origNodes+=area->roles[r].nodes.size();
      }*/

      size_t r=0;
      while (r<area->roles.size()) {
        polygon.TransformArea(projection,
                              optimizeWayMethod,
                              area->roles[r].nodes,
                              8.0);

        polygon.GetBoundingBox(xmin,ymin,xmax,ymax);

        if (polygon.IsEmpty() ||
            (xmax-xmin<=25.0 &&
             ymax-ymin<=25.0)) {
          // We drop all sub roles of the current role, too
          size_t s=r;

          while (s+1<area->roles.size() &&
                 area->roles[s+1].ring>area->roles[r].ring) {
            s++;
          }

          r=s+1;
          continue;
        }

        newRoles.push_back(area->roles[r]);

        newRoles.back().nodes.clear();

        for (size_t i=polygon.GetStart();
             i<=polygon.GetEnd();
             i++) {
          if (polygon.points[i].draw) {
            newRoles.back().nodes.push_back(area->roles[r].nodes[i]);
          }
        }

        r++;
      }

      if (newRoles.empty()) {
        continue;
      }

      AreaRef copiedArea=new Area(*area);

      copiedArea->roles=newRoles;

      optimizedAreas.push_back(copiedArea);
    }

    //progress.Info("Areas: "+NumberToString(origAreas)+" => "+NumberToString(areas.size())+" roles: "+NumberToString(origRoles)+" => "+NumberToString(finalRoles)+ " nodes: "+NumberToString(origNodes)+" => "+NumberToString(finalNodes));
  }


  void OptimizeLowZoomGenerator::GetAreaIndexLevel(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const std::list<AreaRef>& areas,
                                                  TypeData& typeData)
  {
    size_t level=parameter.GetOptimizationMinMag();

    while (true) {
      double                 cellWidth=360.0/pow(2.0,(int)level);
      double                 cellHeight=180.0/pow(2.0,(int)level);
      std::map<Pixel,size_t> cellFillCount;

      for (std::list<AreaRef>::const_iterator a=areas.begin();
          a!=areas.end();
          ++a) {
        AreaRef area=*a;
        // Count number of entries per current type and coordinate
        double minLon;
        double maxLon;
        double minLat;
        double maxLat;

        area->GetBoundingBox(minLon,maxLon,minLat,maxLat);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormated coordinate space (everything is >=0)
        //
        uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
        uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
        uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
        uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

        for (uint32_t y=minyc; y<=maxyc; y++) {
          for (uint32_t x=minxc; x<=maxxc; x++) {
            cellFillCount[Pixel(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      size_t entryCount=0;
      size_t max=0;

      for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount.begin();
           cell!=cellFillCount.end();
           ++cell) {
        entryCount+=cell->second;
        max=std::max(max,cell->second);
      }

      double average=entryCount*1.0/cellFillCount.size();

      if (!(max>parameter.GetOptimizationCellSizeMax() ||
           average>parameter.GetOptimizationCellSizeAverage())) {
        typeData.indexLevel=(uint32_t)level;
        typeData.indexCells=cellFillCount.size();
        typeData.indexEntries=0;

        if (!cellFillCount.empty()) {
          typeData.cellXStart=cellFillCount.begin()->first.x;
          typeData.cellYStart=cellFillCount.begin()->first.y;

          typeData.cellXEnd=typeData.cellXStart;
          typeData.cellYEnd=typeData.cellYStart;

          for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount.begin();
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

  bool OptimizeLowZoomGenerator::WriteAreas(Progress& progress,
                                            FileWriter& writer,
                                            const std::list<AreaRef>& areas,
                                            FileOffsetFileOffsetMap& offsets)
  {
    for (std::list<AreaRef>::const_iterator a=areas.begin();
        a!=areas.end();
        a++) {
      AreaRef    area(*a);
      FileOffset offset;

      writer.GetPos(offset);

      offsets[area->GetFileOffset()]=offset;

      if (!area->WriteOptimized(writer)) {
        return false;
      }
    }

    return true;
  }

  bool OptimizeLowZoomGenerator::WriteAreaBitmap(Progress& progress,
                                                 FileWriter& writer,
                                                 const TypeInfo& type,
                                                 const std::list<AreaRef>& areas,
                                                 const FileOffsetFileOffsetMap& offsets,
                                                 TypeData& data)
  {
    // We do not write a bitmap, if there is not data to map
    if (areas.empty()) {
      return true;
    }

    double                                 cellWidth=360.0/pow(2.0,(int)data.indexLevel);
    double                                 cellHeight=180.0/pow(2.0,(int)data.indexLevel);
    std::map<Pixel,std::list<FileOffset> > cellOffsets;

    for (std::list<AreaRef>::const_iterator a=areas.begin();
        a!=areas.end();
        a++) {
      AreaRef                         area(*a);
      double                          minLon;
      double                          maxLon;
      double                          minLat;
      double                          maxLat;
      FileOffsetFileOffsetMap::const_iterator offset=offsets.find(area->GetFileOffset());

      if (offset==offsets.end()) {
        continue;
      }

      area->GetBoundingBox(minLon,maxLon,minLat,maxLat);

      //
      // Calculate minimum and maximum tile ids that are covered
      // by the way
      // Renormated coordinate space (everything is >=0)
      //
      uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
      uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
      uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
      uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

      for (uint32_t y=minyc; y<=maxyc; y++) {
        for (uint32_t x=minxc; x<=maxxc; x++) {
          cellOffsets[Pixel(x,y)].push_back(offset->second);
        }
      }
    }

    size_t indexEntries=0;
    size_t dataSize=0;
    char   buffer[10];

    for (std::map<Pixel,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      indexEntries+=cell->second.size();

      dataSize+=EncodeNumber(cell->second.size(),buffer);

      FileOffset previousOffset=0;
      for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
           offset!=cell->second.end();
           ++offset) {
        FileOffset data=*offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=*offset;
      }
    }

    data.dataOffsetBytes=BytesNeeededToAddressFileData(dataSize);

    progress.Info("Writing map for level "+
                  NumberToString(data.indexLevel)+", "+
                  NumberToString(cellOffsets.size())+" cells, "+
                  NumberToString(indexEntries)+" entries, "+
                  ByteSizeToString(1.0*data.cellXCount*data.cellYCount*data.dataOffsetBytes+dataSize));

    if (!writer.GetPos(data.bitmapOffset)) {
      progress.Error("Cannot get type index start position in file");
      return false;
    }

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<data.cellXCount*data.cellYCount; i++) {
      FileOffset cellOffset=0;

      writer.WriteFileOffset(cellOffset,
                             data.dataOffsetBytes);
    }

    FileOffset dataStartOffset;

    if (!writer.GetPos(dataStartOffset)) {
      progress.Error("Cannot get start of data section after bitmap");
      return false;
    }

    // Now write the list of offsets of objects for every cell with content
    for (std::map<Pixel,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      FileOffset bitmapCellOffset=data.bitmapOffset+
                                  ((cell->first.y-data.cellYStart)*data.cellXCount+
                                   cell->first.x-data.cellXStart)*data.dataOffsetBytes;
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

      writer.WriteFileOffset(cellOffset-dataStartOffset,
                             data.dataOffsetBytes);

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

  bool OptimizeLowZoomGenerator::GetWays(const ImportParameter& parameter,
                                         Progress& progress,
                                         FileScanner& scanner,
                                         std::set<TypeId>& types,
                                         std::vector<std::list<WayRef> >& ways)
  {
    uint32_t         wayCount=0;
    size_t           collectedWaysCount=0;
    std::set<TypeId> currentTypes(types);

    progress.SetAction("Collecting way data to optimize");

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

      if (currentTypes.find(way->GetType())!=currentTypes.end() &&
          way->nodes.size()>=2) {
        ways[way->GetType()].push_back(way);

        collectedWaysCount++;

        while (collectedWaysCount>parameter.GetOptimizationMaxWayCount() &&
               currentTypes.size()>1) {
          size_t victimType=ways.size();

          for (size_t i=0; i<ways.size(); i++) {
            if (ways[i].size()>0 &&
                (victimType>=ways.size() ||
                 ways[i].size()<ways[victimType].size())) {
              victimType=i;
            }
          }

          if (victimType<ways.size()) {
            collectedWaysCount-=ways[victimType].size();
            ways[victimType].clear();
            currentTypes.erase(victimType);
          }
        }
      }
    }

    for (std::set<TypeId>::const_iterator type=currentTypes.begin();
         type!=currentTypes.end();
         ++type) {
      types.erase(*type);
    }

    progress.Info("Collected "+NumberToString(collectedWaysCount)+" ways for "+NumberToString(currentTypes.size())+" types");

    return true;
  }

  void OptimizeLowZoomGenerator::MergeWays(Progress& progress,
                                           const std::list<WayRef>& ways,
                                           std::list<WayRef>& newWays)
  {
    std::map<Id, std::list<WayRef > > waysByJoin;
    std::set<FileOffset>              usedWays;

    progress.Info("Merging "+NumberToString(ways.size())+" ways");

    for (std::list<WayRef>::const_iterator way=ways.begin();
        way!=ways.end();
        way++) {
      if ((*way)->ids.front()!=0) {
        waysByJoin[(*way)->ids.front()].push_back(*way);
      }

      if ((*way)->ids.back()!=0) {
        waysByJoin[(*way)->ids.back()].push_back(*way);
      }
    }

    for (std::map<Id, std::list<WayRef> >::iterator entry=waysByJoin.begin();
        entry!=waysByJoin.end();
        entry++) {
      while (!entry->second.empty()) {

        WayRef way=entry->second.front();

        entry->second.pop_front();

        if (usedWays.find(way->GetFileOffset())!=usedWays.end()) {
          continue;
        }

        usedWays.insert(way->GetFileOffset());

        while (true) {
          std::map<Id, std::list<WayRef> >::iterator match;

          match=waysByJoin.find(way->ids.front());
          if (match!=waysByJoin.end()) {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetFileOffset())!=usedWays.end() ||
                    way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
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
              std::vector<Id>       newIds;
              std::vector<GeoCoord> newNodes;

              newIds.reserve(way->ids.size()+(*otherWay)->ids.size()-1);
              newNodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->ids.front()==(*otherWay)->ids.front()) {
                for (size_t i=(*otherWay)->nodes.size()-1; i>0; i--) {
                  newIds.push_back((*otherWay)->ids[i]);
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=0; i<way->nodes.size(); i++) {
                  newIds.push_back(way->ids[i]);
                  newNodes.push_back(way->nodes[i]);
                }

                way->ids=newIds;
                way->nodes=newNodes;
              }
              else {
                for (size_t i=0; i<(*otherWay)->nodes.size(); i++) {
                  newIds.push_back((*otherWay)->ids[i]);
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=1; i<way->nodes.size(); i++) {
                  newIds.push_back(way->ids[i]);
                  newNodes.push_back(way->nodes[i]);
                }

                way->ids=newIds;
                way->nodes=newNodes;
              }

              usedWays.insert((*otherWay)->GetFileOffset());
              match->second.erase(otherWay);

              continue;
            }
          }

          match=waysByJoin.find(way->ids.back());
          if (match!=waysByJoin.end()) {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetFileOffset())!=usedWays.end() ||
                     way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
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
              way->ids.reserve(way->ids.size()+(*otherWay)->ids.size()-1);
              way->nodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->ids.back()==(*otherWay)->ids.front()) {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  way->ids.push_back((*otherWay)->ids[i]);
                  way->nodes.push_back((*otherWay)->nodes[i]);
                }
              }
              else {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  size_t idx=(*otherWay)->nodes.size()-1-i;

                  way->ids.push_back((*otherWay)->ids[idx]);
                  way->nodes.push_back((*otherWay)->nodes[idx]);
                }
              }

              usedWays.insert((*otherWay)->GetFileOffset());
              match->second.erase(otherWay);

              continue;
            }
          }

          break;
        }

        newWays.push_back(new Way(*way));
      }
    }

    progress.Info("Merged to "+NumberToString(newWays.size())+" ways");
  }

  void OptimizeLowZoomGenerator::GetWayIndexLevel(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const std::list<WayRef>& ways,
                                                  TypeData& typeData)
  {
    size_t level=parameter.GetOptimizationMinMag();

    while (true) {
      double                 cellWidth=360.0/pow(2.0,(int)level);
      double                 cellHeight=180.0/pow(2.0,(int)level);
      std::map<Pixel,size_t> cellFillCount;

      for (std::list<WayRef>::const_iterator w=ways.begin();
          w!=ways.end();
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
        uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
        uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
        uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
        uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

        for (uint32_t y=minyc; y<=maxyc; y++) {
          for (uint32_t x=minxc; x<=maxxc; x++) {
            cellFillCount[Pixel(x,y)]++;
          }
        }
      }

      // Check if cell fill for current type is in defined limits
      size_t entryCount=0;
      size_t max=0;

      for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount.begin();
           cell!=cellFillCount.end();
           ++cell) {
        entryCount+=cell->second;
        max=std::max(max,cell->second);
      }

      double average=entryCount*1.0/cellFillCount.size();

      if (!(max>parameter.GetOptimizationCellSizeMax() ||
           average>parameter.GetOptimizationCellSizeAverage())) {
        typeData.indexLevel=(uint32_t)level;
        typeData.indexCells=cellFillCount.size();
        typeData.indexEntries=0;

        if (!cellFillCount.empty()) {
          typeData.cellXStart=cellFillCount.begin()->first.x;
          typeData.cellYStart=cellFillCount.begin()->first.y;

          typeData.cellXEnd=typeData.cellXStart;
          typeData.cellYEnd=typeData.cellYStart;

          for (std::map<Pixel,size_t>::const_iterator cell=cellFillCount.begin();
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

  void OptimizeLowZoomGenerator::OptimizeWays(Progress& progress,
                                              std::list<WayRef>& ways,
                                              size_t width,
                                              size_t height,
                                              const Magnification& magnification,
                                              TransPolygon::OptimizeMethod optimizeWayMethod)
  {
    MercatorProjection projection;

    projection.Set(0,0,magnification,width,height);

    std::list<WayRef>::iterator w=ways.begin();

    while (w!=ways.end()) {
      WayRef                way(*w);
      TransPolygon          polygon;
      std::vector<GeoCoord> newNodes;
      double                xmin;
      double                xmax;
      double                ymin;
      double                ymax;

      polygon.TransformWay(projection,
                           optimizeWayMethod,
                           way->nodes,
                           8.0);

      polygon.GetBoundingBox(xmin,ymin,xmax,ymax);

      if (xmax-xmin<=1.0 &&
          ymax-ymin<=1.0) {
        w=ways.erase(w);
        continue;
      }

      newNodes.reserve(polygon.GetLength());

      for (size_t i=polygon.GetStart();
           i<=polygon.GetEnd();
           i++) {
        if (polygon.points[i].draw) {
          newNodes.push_back(way->nodes[i]);
        }
      }

      way->nodes=newNodes;

      ++w;
    }
  }

  bool OptimizeLowZoomGenerator::WriteWays(Progress& progress,
                                           FileWriter& writer,
                                           const std::list<WayRef>& ways,
                                           FileOffsetFileOffsetMap& offsets)
  {
    for (std::list<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        w++) {
      WayRef     way(*w);
      FileOffset offset;

      writer.GetPos(offset);

      offsets[way->GetFileOffset()]=offset;

      if (!way->WriteOptimized(writer)) {
        return false;
      }
    }

    return true;
  }

  bool OptimizeLowZoomGenerator::WriteWayBitmap(Progress& progress,
                                                FileWriter& writer,
                                                const TypeInfo& type,
                                                const std::list<WayRef>& ways,
                                                const FileOffsetFileOffsetMap& offsets,
                                                TypeData& data)
  {
    // We do not write a bitmap, if there is not data to map
    if (ways.empty()) {
      return true;
    }

    double                                 cellWidth=360.0/pow(2.0,(int)data.indexLevel);
    double                                 cellHeight=180.0/pow(2.0,(int)data.indexLevel);
    std::map<Pixel,std::list<FileOffset> > cellOffsets;

    for (std::list<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        w++) {
      WayRef                          way(*w);
      double                          minLon;
      double                          maxLon;
      double                          minLat;
      double                          maxLat;
      FileOffsetFileOffsetMap::const_iterator offset=offsets.find(way->GetFileOffset());

      if (offset==offsets.end()) {
        continue;
      }

      way->GetBoundingBox(minLon,maxLon,minLat,maxLat);

      //
      // Calculate minimum and maximum tile ids that are covered
      // by the way
      // Renormated coordinate space (everything is >=0)
      //
      uint32_t minxc=(uint32_t)floor((minLon+180.0)/cellWidth);
      uint32_t maxxc=(uint32_t)floor((maxLon+180.0)/cellWidth);
      uint32_t minyc=(uint32_t)floor((minLat+90.0)/cellHeight);
      uint32_t maxyc=(uint32_t)floor((maxLat+90.0)/cellHeight);

      for (uint32_t y=minyc; y<=maxyc; y++) {
        for (uint32_t x=minxc; x<=maxxc; x++) {
          cellOffsets[Pixel(x,y)].push_back(offset->second);
        }
      }
    }

    size_t indexEntries=0;
    size_t dataSize=0;
    char   buffer[10];

    for (std::map<Pixel,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      indexEntries+=cell->second.size();

      dataSize+=EncodeNumber(cell->second.size(),buffer);

      FileOffset previousOffset=0;
      for (std::list<FileOffset>::const_iterator offset=cell->second.begin();
           offset!=cell->second.end();
           ++offset) {
        FileOffset data=*offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=*offset;
      }
    }

    data.dataOffsetBytes=BytesNeeededToAddressFileData(dataSize);

    progress.Info("Writing map level "+
                  NumberToString(data.indexLevel)+", "+
                  NumberToString(cellOffsets.size())+" cells, "+
                  NumberToString(indexEntries)+" entries, "+
                  ByteSizeToString(1.0*data.cellXCount*data.cellYCount*data.dataOffsetBytes+dataSize));

    if (!writer.GetPos(data.bitmapOffset)) {
      progress.Error("Cannot get type index start position in file");
      return false;
    }

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<data.cellXCount*data.cellYCount; i++) {
      FileOffset cellOffset=0;

      writer.WriteFileOffset(cellOffset,
                             data.dataOffsetBytes);
    }

    FileOffset dataStartOffset;

    if (!writer.GetPos(dataStartOffset)) {
      progress.Error("Cannot get start of data section after bitmap");
      return false;
    }

    // Now write the list of offsets of objects for every cell with content
    for (std::map<Pixel,std::list<FileOffset> >::const_iterator cell=cellOffsets.begin();
         cell!=cellOffsets.end();
         ++cell) {
      FileOffset bitmapCellOffset=data.bitmapOffset+
                                  ((cell->first.y-data.cellYStart)*data.cellXCount+
                                   cell->first.x-data.cellXStart)*data.dataOffsetBytes;
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

      writer.WriteFileOffset(cellOffset-dataStartOffset,
                             data.dataOffsetBytes);

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

  bool OptimizeLowZoomGenerator::HandleAreas(const ImportParameter& parameter,
                                             Progress& progress,
                                             const TypeConfig& typeConfig,
                                             FileWriter& writer,
                                             const std::set<TypeId>& types,
                                             std::list<TypeData>& typesData)
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'areas.dat'");
      return false;
    }

    std::set<TypeId>                 typesToProcess(types);
    std::vector<std::list<AreaRef> > allAreas(typeConfig.GetTypes().size());

    while (true) {
      //
      // Load type data
      //

      if (!GetAreas(parameter,
                    progress,
                    scanner,
                    typesToProcess,
                    allAreas)) {
        return false;
      }

      for (size_t type=0; type<allAreas.size(); type++) {
        if (allAreas[type].empty()) {
          continue;
        }

        progress.SetAction("Optimizing type "+ typeConfig.GetTypeInfo(type).GetName()+" ("+NumberToString(type)+")");

        size_t origAreas=allAreas[type].size();
        size_t origRoles=0;
        size_t origNodes=0;

        for (std::list<AreaRef>::const_iterator a=allAreas[type].begin();
            a!=allAreas[type].end();
            ++a) {
          AreaRef area=*a;

          origRoles+=area->roles.size();

          for (size_t r=0; r<area->roles.size(); r++) {
            origNodes+=area->roles[r].nodes.size();
          }
        }

        for (uint32_t level=parameter.GetOptimizationMinMag();
             level<=parameter.GetOptimizationMaxMag();
             level++) {
          Magnification      magnification; // Magnification, we optimize for
          std::list<AreaRef> optimizedAreas;

          magnification.SetLevel(level);

          progress.Info("Optimizing type for magnification level "+NumberToString(magnification.GetLevel()));

          OptimizeAreas(progress,
                        typeConfig,
                        type,
                        allAreas[type],
                        optimizedAreas,
                        800,640,
                        magnification,
                        parameter.GetOptimizationWayMethod());

          if (optimizedAreas.empty()) {
            progress.Info("Empty optimization result, no index generated");
            continue;
          }

          size_t optAreas=optimizedAreas.size();
          size_t optRoles=0;
          size_t optNodes=0;

          for (std::list<AreaRef>::const_iterator a=optimizedAreas.begin();
              a!=optimizedAreas.end();
              ++a) {
            AreaRef area=*a;

            optRoles+=area->roles.size();

            for (size_t r=0; r<area->roles.size(); r++) {
              optNodes+=area->roles[r].nodes.size();
            }
          }

          /*
          std::cout << "Areas: " << origAreas << " => " << optAreas << std::endl;
          std::cout << "Roles: " << origRoles << " => " << optRoles << std::endl;
          std::cout << "Nodes: " << origNodes << " => " << optNodes << std::endl;*/

          TypeData typeData;

          typeData.type=type;
          typeData.optLevel=level;

          GetAreaIndexLevel(parameter,
                            progress,
                            optimizedAreas,
                            typeData);

          //std::cout << "Resulting index level: " << typeData.indexLevel << ", " << typeData.indexCells << ", " << typeData.indexEntries << std::endl;

          FileOffsetFileOffsetMap offsets;

          if (!WriteAreas(progress,
                          writer,
                          optimizedAreas,
                          offsets)) {
            return false;
          }

          if (!WriteAreaBitmap(progress,
                               writer,
                               typeConfig.GetTypeInfo(type),
                               optimizedAreas,
                               offsets,
                               typeData)) {
            return false;
          }

          typesData.push_back(typeData);
        }

        allAreas[type].clear();
      }

      if (typesToProcess.empty()) {
        break;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool OptimizeLowZoomGenerator::HandleWays(const ImportParameter& parameter,
                                            Progress& progress,
                                            const TypeConfig& typeConfig,
                                            FileWriter& writer,
                                            const std::set<TypeId>& types,
                                            std::vector<TypeData>& typesData)
  {
    FileScanner   scanner;
    Magnification magnification; // Magnification, we optimize for

    magnification.SetLevel((uint32_t)parameter.GetOptimizationMaxMag());

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    std::set<TypeId>                typesToProcess(types);
    std::vector<std::list<WayRef> > allWays(typeConfig.GetTypes().size());

    while (true) {
      //
      // Load type data
      //

      if (!GetWays(parameter,
                   progress,
                   scanner,
                   typesToProcess,
                   allWays)) {
        return false;
      }

      for (size_t type=0; type<allWays.size(); type++) {
        if (allWays[type].empty()) {
          continue;
        }

        progress.SetAction("Handling type "+ typeConfig.GetTypeInfo(type).GetName());

        //
        // Join ways
        //

        std::list<WayRef> newWays;

        MergeWays(progress,
                  allWays[type],
                  newWays);

        allWays[type].clear();

        if (newWays.empty()) {
          continue;
        }

        //
        // Transform/Optimize the way and store it
        //

        // TODO: Wee need to make import parameters for the width and the height
        OptimizeWays(progress,
                     newWays,
                     800,640,
                     magnification,
                     parameter.GetOptimizationWayMethod());

        typesData[type].type=type;
        typesData[type].optLevel=0;

        GetWayIndexLevel(parameter,
                         progress,
                         newWays,
                         typesData[type]);


        FileOffsetFileOffsetMap offsets;

        if (!WriteWays(progress,
                       writer,
                       newWays,
                       offsets)) {
          return false;
        }

        if (!WriteWayBitmap(progress,
                            writer,
                            typeConfig.GetTypeInfo(type),
                            newWays,
                            offsets,
                            typesData[type])) {
          return false;
        }
      }

      if (typesToProcess.empty()) {
        break;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool OptimizeLowZoomGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    FileOffset                  indexOffset=0;
    FileWriter                  writer;
    Magnification               magnification; // Magnification, we optimize for
    std::set<TypeId>            areaTypes;         // Types we optimize
    std::list<TypeData>         areaTypesData;
    std::set<TypeId>            wayTypes;         // Types we optimize
    std::vector<TypeData>       wayTypesData;

    GetAreaTypesToOptimize(typeConfig,areaTypes);
    GetWayTypesToOptimize(typeConfig,wayTypes);

    wayTypesData.resize(typeConfig.GetMaxTypeId()+1);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "optimized.dat"))) {
      progress.Error("Cannot create 'relations.dat'");
      return false;
    }

    //
    // Write header
    //

    writer.WriteFileOffset(indexOffset);

    if (!HandleAreas(parameter,
                     progress,
                     typeConfig,
                     writer,
                     areaTypes,
                     areaTypesData)) {
      progress.Error("Error while optimizing areas");
      return false;
    }

    if (!HandleWays(parameter,
                    progress,
                    typeConfig,
                    writer,
                    wayTypes,
                    wayTypesData)) {
      progress.Error("Error while optimizing ways");
      return false;
    }

    if (!writer.GetPos(indexOffset)) {
      progress.Error("Cannot read index start position");
      return false;
    }

    if (!WriteHeader(writer,
                     areaTypesData,
                     wayTypes,
                     wayTypesData,
                    (uint32_t)parameter.GetOptimizationMaxMag())) {
      progress.Error("Cannot write file header");
      return false;
    }

    if (!writer.SetPos(0)) {
      progress.Error("Cannot read index offset");
    }

    if (!writer.WriteFileOffset(indexOffset)) {
      progress.Error("Cannot write index position");
    }

    return true;
  }
}
