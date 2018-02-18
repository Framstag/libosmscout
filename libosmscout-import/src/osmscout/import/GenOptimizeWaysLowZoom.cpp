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

#include <osmscout/import/GenOptimizeWaysLowZoom.h>

#include <osmscout/Pixel.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/Way.h>

#include <osmscout/WayDataFile.h>
#include <osmscout/OptimizeWaysLowZoom.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

namespace osmscout
{
  OptimizeWaysLowZoomGenerator::TypeData::TypeData()
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

  void OptimizeWaysLowZoomGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                    ImportModuleDescription& description) const
  {
    description.SetName("OptimizeWaysLowZoomGenerator");
    description.SetDescription("Create index for area lookup of reduced resolution ways");

    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedOptionalFile(OptimizeWaysLowZoom::FILE_WAYSOPT_DAT);
  }

  void OptimizeWaysLowZoomGenerator::GetWayTypesToOptimize(const TypeConfig& typeConfig,
                                                           std::set<TypeInfoRef>& types)
  {
    for (auto &type : typeConfig.GetWayTypes()) {
      if (!type->GetIgnore() &&
          !type->IsInternal() &&
          type->GetOptimizeLowZoom()) {
        types.insert(type);
      }
    }
  }

  bool OptimizeWaysLowZoomGenerator::WriteTypeData(FileWriter& writer,
                                                   const TypeData& data)
  {
    assert(data.type);

    writer.Write(data.type->GetWayId());
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

  bool OptimizeWaysLowZoomGenerator::WriteHeader(FileWriter& writer,
                                                 const std::list<TypeData>& wayTypesData,
                                                 uint32_t optimizeMaxMap)
  {
    writer.Write(optimizeMaxMap);
    writer.Write((uint32_t)wayTypesData.size());

    for (const auto &typeData : wayTypesData) {
      if (!WriteTypeData(writer,
                         typeData)) {
        return false;
      }
    }

    return true;
  }

  bool OptimizeWaysLowZoomGenerator::GetWays(const TypeConfig& typeConfig,
                                             const ImportParameter& parameter,
                                             Progress& progress,
                                             FileScanner& scanner,
                                             std::set<TypeInfoRef>& types,
                                             std::vector<std::list<WayRef> >& ways)
  {
    uint32_t              wayCount=0;
    size_t                collectedWaysCount=0;
    std::set<TypeInfoRef> currentTypes(types);
    FeatureRef            featureRef(typeConfig.GetFeature(RefFeature::NAME));

    progress.SetAction("Collecting way data to optimize");

    scanner.GotoBegin();

    scanner.Read(wayCount);

    for (uint32_t w=1; w<=wayCount; w++) {
      WayRef way=std::make_shared<Way>();

      progress.SetProgress(w,
                           wayCount);

      way->Read(typeConfig,
                scanner);

      if (currentTypes.find(way->GetType())==currentTypes.end()) {
        continue;
      }

      for (size_t f=0; f<way->GetFeatureCount(); f++) {
        if (way->GetFeature(f).GetFeature()!=featureRef &&
            way->HasFeature(f)) {
          way->UnsetFeature(f);
        }
      }

      ways[way->GetType()->GetIndex()].push_back(way);

      collectedWaysCount++;

      while (collectedWaysCount>parameter.GetOptimizationMaxWayCount() &&
             currentTypes.size()>1) {
        TypeInfoRef victimType;

        for (auto &type : currentTypes) {
          if (ways[type->GetIndex()].size()>0 &&
              (!victimType ||
               ways[type->GetIndex()].size()<ways[victimType->GetIndex()].size())) {
            victimType=type;
          }
        }

        assert(victimType);

        collectedWaysCount-=ways[victimType->GetIndex()].size();
        ways[victimType->GetIndex()].clear();
        currentTypes.erase(victimType);
      }
    }

    for (auto &type : currentTypes) {
      types.erase(type);
    }

    progress.Info("Collected "+std::to_string(collectedWaysCount)+" ways for "+std::to_string(currentTypes.size())+" types");

    return !scanner.HasError();
  }

  void OptimizeWaysLowZoomGenerator::MergeWays(Progress& progress,
                                               const std::list<WayRef>& ways,
                                               std::list<WayRef>& newWays)
  {
    std::map<Id, std::list<WayRef > > waysByJoin;
    std::set<FileOffset>              usedWays;

    progress.Info("Merging "+std::to_string(ways.size())+" ways");

    for (const auto &way : ways) {
      if (way->GetFrontId()!=0) {
        waysByJoin[way->GetFrontId()].push_back(way);
      }

      if (way->GetBackId()!=0) {
        waysByJoin[way->GetBackId()].push_back(way);
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

          match=waysByJoin.find(way->GetFrontId());
          if (match!=waysByJoin.end()) {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetFileOffset())!=usedWays.end() ||
                    way->GetFeatureValueBuffer()!=(*otherWay)->GetFeatureValueBuffer())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
                      way->GetFeatureValueBuffer()!=(*stillOtherWay)->GetFeatureValueBuffer())) {
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

              if (way->GetFrontId()==(*otherWay)->GetFrontId()) {
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

              usedWays.insert((*otherWay)->GetFileOffset());
              match->second.erase(otherWay);

              continue;
            }
          }

          match=waysByJoin.find(way->GetBackId());
          if (match!=waysByJoin.end()) {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetFileOffset())!=usedWays.end() ||
                     way->GetFeatureValueBuffer()!=(*otherWay)->GetFeatureValueBuffer())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
                      way->GetFeatureValueBuffer()!=(*stillOtherWay)->GetFeatureValueBuffer())) {
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

              if (way->GetBackId()==(*otherWay)->GetFrontId()) {
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

              usedWays.insert((*otherWay)->GetFileOffset());
              match->second.erase(otherWay);

              continue;
            }
          }

          break;
        }

        newWays.push_back(std::make_shared<Way>(*way));
      }
    }

    progress.Info("Merged to "+std::to_string(newWays.size())+" ways");
  }

  void OptimizeWaysLowZoomGenerator::GetWayIndexLevel(const ImportParameter& parameter,
                                                      const std::list<WayRef>& ways,
                                                      TypeData& typeData)
  {
    size_t level=1;//parameter.GetOptimizationMinMag();

    while (true) {
      std::map<Pixel,size_t> cellFillCount;

      for (const auto& way : ways) {
        GeoBox boundingBox;

        // Count number of entries per current type and coordinate

        way->GetBoundingBox(boundingBox);

        //
        // Calculate minimum and maximum tile ids that are covered
        // by the way
        // Renormated coordinate space (everything is >=0)
        //
        uint32_t minxc=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/cellDimension[level].width);
        uint32_t maxxc=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/cellDimension[level].width);
        uint32_t minyc=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/cellDimension[level].height);
        uint32_t maxyc=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/cellDimension[level].height);

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

          for (const auto& cell : cellFillCount) {
            typeData.indexEntries+=cell.second;

            typeData.cellXStart=std::min(typeData.cellXStart,cell.first.x);
            typeData.cellXEnd=std::max(typeData.cellXEnd,cell.first.x);

            typeData.cellYStart=std::min(typeData.cellYStart,cell.first.y);
            typeData.cellYEnd=std::max(typeData.cellYEnd,cell.first.y);
          }
        }

        typeData.cellXCount=typeData.cellXEnd-typeData.cellXStart+1;
        typeData.cellYCount=typeData.cellYEnd-typeData.cellYStart+1;

        return;
      }

      level++;
    }
  }

  void OptimizeWaysLowZoomGenerator::OptimizeWays(Progress& progress,
                                                  const std::list<WayRef>& ways,
                                                  std::list<WayRef>& optimizedWays,
                                                  size_t width,
                                                  size_t height,
                                                  double dpi,
                                                  double pixel,
                                                  const Magnification& magnification,
                                                  TransPolygon::OptimizeMethod optimizeWayMethod)
  {
    MercatorProjection projection;

    projection.Set(GeoCoord(0.0,0.0),magnification,dpi,width,height);

    for (auto &way :ways) {
      TransPolygon       polygon;
      std::vector<Point> newNodes;
      double             xmin;
      double             xmax;
      double             ymin;
      double             ymax;

      polygon.TransformWay(projection,
                           optimizeWayMethod,
                           way->nodes,
                           pixel/8);

      polygon.GetBoundingBox(xmin,ymin,xmax,ymax);

      if (xmax-xmin<=pixel &&
          ymax-ymin<=pixel) {
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

      WayRef copiedWay=std::make_shared<Way>(*way);

      copiedWay->nodes=newNodes;

      if (!IsValidToWrite(copiedWay->nodes)) {
        progress.Error("Way coordinates are not dense enough to be written for way "+
                       std::to_string(way->GetFileOffset()));
        continue;
      }

      optimizedWays.push_back(copiedWay);
    }
  }

  void OptimizeWaysLowZoomGenerator::WriteWays(const TypeConfig& typeConfig,
                                               FileWriter& writer,
                                               const std::list<WayRef>& ways,
                                               FileOffsetFileOffsetMap& offsets)
  {
    for (const auto &way : ways) {
      offsets[way->GetFileOffset()]=writer.GetPos();

      way->WriteOptimized(typeConfig,
                          writer);
    }
  }

  bool OptimizeWaysLowZoomGenerator::WriteWayBitmap(Progress& progress,
                                                    FileWriter& writer,
                                                    const std::list<WayRef>& ways,
                                                    const FileOffsetFileOffsetMap& offsets,
                                                    TypeData& data)
  {
    // We do not write a bitmap, if there is not data to map
    if (ways.empty()) {
      return true;
    }

    double                                 cellWidth=cellDimension[data.indexLevel].width;
    double                                 cellHeight=cellDimension[data.indexLevel].height;
    std::map<Pixel,std::list<FileOffset> > cellOffsets;

    for (const auto &way : ways) {
      GeoBox                                  boundingBox;
      FileOffsetFileOffsetMap::const_iterator offset=offsets.find(way->GetFileOffset());

      if (offset==offsets.end()) {
        continue;
      }

      way->GetBoundingBox(boundingBox);

      //
      // Calculate minimum and maximum tile ids that are covered
      // by the way
      // Renormated coordinate space (everything is >=0)
      //
      uint32_t minxc=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/cellWidth);
      uint32_t maxxc=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/cellWidth);
      uint32_t minyc=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/cellHeight);
      uint32_t maxyc=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/cellHeight);

      for (uint32_t y=minyc; y<=maxyc; y++) {
        for (uint32_t x=minxc; x<=maxxc; x++) {
          cellOffsets[Pixel(x,y)].push_back(offset->second);
        }
      }
    }

    size_t indexEntries=0;
    size_t dataSize=1; // Actual data will be prefixed by one empty byte
    char   buffer[10];

    for (const auto& cell : cellOffsets) {
      indexEntries+=cell.second.size();

      dataSize+=EncodeNumber(cell.second.size(),buffer);

      FileOffset previousOffset=0;
      for (const auto& offset : cell.second) {
        FileOffset data=offset-previousOffset;

        dataSize+=EncodeNumber(data,buffer);

        previousOffset=offset;
      }
    }

    data.dataOffsetBytes=BytesNeededToEncodeNumber(dataSize);

    progress.Info("Writing map for level "+
                  std::to_string(data.optLevel)+", index level "+
                  std::to_string(data.indexLevel)+", "+
                  std::to_string(cellOffsets.size())+" cells, "+
                  std::to_string(indexEntries)+" entries, "+
                  ByteSizeToString(1.0*data.cellXCount*data.cellYCount*data.dataOffsetBytes+dataSize));

    data.bitmapOffset=writer.GetPos();

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only override cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<data.cellXCount*data.cellYCount; i++) {
      writer.WriteFileOffset((FileOffset)0,
                             data.dataOffsetBytes);
    }

    FileOffset dataStartOffset=writer.GetPos();

    // Move data start by one byte. It creates little bit larger output file.
    // But without it 0 is valid cell offset and these data will not be visible,
    // because for reader means that this cell has no data!

    // TODO: when data format will be changing, consider usage ones (0xFF..FF) as empty placeholder
    writer.WriteFileOffset((FileOffset)0,1);

    // Now write the list of offsets of objects for every cell with content
    for (const auto& cell : cellOffsets) {
      FileOffset bitmapCellOffset=data.bitmapOffset+
                                  ((cell.first.y-data.cellYStart)*data.cellXCount+
                                   cell.first.x-data.cellXStart)*data.dataOffsetBytes;
      FileOffset previousOffset=0;
      FileOffset cellOffset;

      cellOffset=writer.GetPos();

      writer.SetPos(bitmapCellOffset);

      writer.WriteFileOffset(cellOffset-dataStartOffset,
                             data.dataOffsetBytes);

      writer.SetPos(cellOffset);

      writer.WriteNumber((uint32_t)cell.second.size());

      for (const auto& offset : cell.second) {
        writer.WriteNumber((FileOffset)(offset-previousOffset));

        previousOffset=offset;
      }
    }

    return !writer.HasError();
  }

  bool OptimizeWaysLowZoomGenerator::HandleWays(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeConfig& typeConfig,
                                                FileWriter& writer,
                                                const std::set<TypeInfoRef>& types,
                                                std::list<TypeData>& typesData)
  {
    FileScanner   scanner;
    Magnification magnification; // Magnification, we optimize for
    // Everything smaller than 2mm should get dropped. Width, height and DPI come from the Nexus 4
    double        dpi=320.0;
    double        pixel=0.5 /* mm */ * dpi / 25.4 /* inch */;

    magnification.SetLevel((uint32_t)parameter.GetOptimizationMaxMag());

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      std::set<TypeInfoRef>           typesToProcess(types);
      std::vector<std::list<WayRef> > allWays(typeConfig.GetTypeCount());

      while (true) {
        //
        // Load type data
        //

        if (!GetWays(typeConfig,
                     parameter,
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

          progress.SetAction("Optimizing type "+ typeConfig.GetTypeInfo(type)->GetName());

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

          for (uint32_t level=parameter.GetOptimizationMinMag();
               level<=parameter.GetOptimizationMaxMag();
               level++) {
            Magnification     magnification; // Magnification, we optimize for
            std::list<WayRef> optimizedWays;

            magnification.SetLevel(level);

            // TODO: Wee need to make import parameters for the width and the height
            OptimizeWays(progress,
                         newWays,
                         optimizedWays,
                         800,480,
                         dpi,
                         pixel,
                         magnification,
                         parameter.GetOptimizationWayMethod());

            if (optimizedWays.empty()) {
              progress.Debug("Empty optimization result for level "+std::to_string(level)+", no index bitmap generated");

              TypeData typeData;

              typeData.type=typeConfig.GetTypeInfo(type);
              typeData.optLevel=level;

              typesData.push_back(typeData);
              continue;
            }

            TypeData typeData;

            typeData.type=typeConfig.GetTypeInfo(type);
            typeData.optLevel=level;

            GetWayIndexLevel(parameter,
                             optimizedWays,
                             typeData);


            FileOffsetFileOffsetMap offsets;

            WriteWays(typeConfig,
                      writer,
                      optimizedWays,
                      offsets);

            if (!WriteWayBitmap(progress,
                                writer,
                                optimizedWays,
                                offsets,
                                typeData)) {
              return false;
            }

            typesData.push_back(typeData);

            optimizedWays.clear();
          }

          newWays.clear();
        }

        if (typesToProcess.empty()) {
          break;
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool OptimizeWaysLowZoomGenerator::Import(const TypeConfigRef& typeConfig,
                                            const ImportParameter& parameter,
                                            Progress& progress)
  {
    FileOffset            indexOffset=0;
    FileWriter            writer;
    Magnification         magnification; // Magnification, we optimize for
    std::set<TypeInfoRef> wayTypes;         // Types we optimize
    std::list<TypeData>   wayTypesData;

    GetWayTypesToOptimize(*typeConfig,
                          wayTypes);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  OptimizeWaysLowZoom::FILE_WAYSOPT_DAT));

      //
      // Write header
      //

      writer.WriteFileOffset(indexOffset);

      if (!HandleWays(parameter,
                      progress,
                      *typeConfig,
                      writer,
                      wayTypes,
                      wayTypesData)) {
        progress.Error("Error while optimizing ways");
        return false;
      }

      indexOffset=writer.GetPos();

      if (!WriteHeader(writer,
                       wayTypesData,
                       (uint32_t)parameter.GetOptimizationMaxMag())) {
        progress.Error("Cannot write file header");
        return false;
      }

      writer.SetPos(0);
      writer.WriteFileOffset(indexOffset);

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
