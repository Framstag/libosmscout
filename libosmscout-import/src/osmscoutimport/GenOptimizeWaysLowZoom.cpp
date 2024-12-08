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

#include <osmscoutimport/GenOptimizeWaysLowZoom.h>

#include <osmscout/Pixel.h>

#include <osmscout/feature/RefFeature.h>

#include <osmscout/Way.h>

#include <osmscout/db/WayDataFile.h>
#include <osmscout/db/OptimizeWaysLowZoom.h>

#include <osmscout/system/Assert.h>

#include <osmscout/projection/MercatorProjection.h>

#include <osmscout/io/File.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/TileId.h>
#include <osmscout/util/Transformation.h>

namespace osmscout
{
  OptimizeWaysLowZoomGenerator::TypeData::TypeData()
  : type(nullptr),
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
    for (const auto &type : typeConfig.GetWayTypes()) {
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
                                                 MagnificationLevel optimizeMaxMap)
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
    size_t                collectedWaysCount=0;
    std::set<TypeInfoRef> currentTypes(types);
    FeatureRef            featureRef(typeConfig.GetFeature(RefFeature::NAME));

    progress.SetAction("Collecting way data to optimize");

    scanner.GotoBegin();

    uint32_t wayCount=scanner.ReadUInt32();

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

        for (const auto &type : currentTypes) {
          if (!ways[type->GetIndex()].empty() &&
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

    for (const auto &type : currentTypes) {
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

    for (auto& entry : waysByJoin) {
      while (!entry.second.empty()) {

        WayRef way=entry.second.front();

        entry.second.pop_front();

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
              ++otherWay;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              auto stillOtherWay=otherWay;

              ++stillOtherWay;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
                      way->GetFeatureValueBuffer()!=(*stillOtherWay)->GetFeatureValueBuffer())) {
                ++stillOtherWay;
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

                for (const auto& node : way->nodes) {
                  newNodes.push_back(node);
                }

                way->nodes=newNodes;
                way->segments.clear();
                way->bbox.Invalidate();
              }
              else {
                for (const auto& node : (*otherWay)->nodes) {
                  newNodes.push_back(node);
                }

                for (size_t i=1; i<way->nodes.size(); i++) {
                  newNodes.push_back(way->nodes[i]);
                }

                way->nodes=newNodes;
                way->segments.clear();
                way->bbox.Invalidate();
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
              ++otherWay;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              auto stillOtherWay=otherWay;

              ++stillOtherWay;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetFileOffset())!=usedWays.end() ||
                      way->GetFeatureValueBuffer()!=(*stillOtherWay)->GetFeatureValueBuffer())) {
                ++stillOtherWay;
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
    MagnificationLevel level(1);

    while (true) {
      Magnification          magnification(level);
      std::map<Pixel,size_t> cellFillCount;

      for (const auto& way : ways) {
        GeoBox boundingBox=way->GetBoundingBox();

        // Count number of entries per current type and coordinate

        TileIdBox box(TileId::GetTile(magnification,boundingBox.GetMinCoord()),
                      TileId::GetTile(magnification,boundingBox.GetMaxCoord()));

        for (const auto& tileId : box) {
          cellFillCount[tileId.AsPixel()]++;
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
        typeData.indexLevel=level;
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

    for (const auto &way :ways) {
      TransBuffer        transBuffer;
      std::vector<Point> newNodes;
      double             xmin;
      double             xmax;
      double             ymin;
      double             ymax;

      TransformWay(way->nodes,
                   transBuffer,
                   projection,
                   optimizeWayMethod,
                   pixel/8);

      transBuffer.GetBoundingBox(xmin,ymin,xmax,ymax);

      if (xmax-xmin<=pixel &&
          ymax-ymin<=pixel) {
        continue;
      }

      newNodes.reserve(transBuffer.GetLength());

      for (size_t i=transBuffer.GetStart();
           i<=transBuffer.GetEnd();
           i++) {
        if (transBuffer.points[i].draw) {
          newNodes.push_back(way->nodes[i]);
        }
      }

      WayRef copiedWay=std::make_shared<Way>(*way);

      copiedWay->nodes=newNodes;
      copiedWay->bbox.Invalidate();
      copiedWay->segments.clear();

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

    Magnification                          magnification(data.indexLevel);
    std::map<Pixel,std::list<FileOffset> > cellOffsets;

    for (const auto &way : ways) {
      auto offset=offsets.find(way->GetFileOffset());

      if (offset==offsets.end()) {
        continue;
      }

      GeoBox boundingBox=way->GetBoundingBox();

      TileIdBox box(TileId::GetTile(magnification,boundingBox.GetMinCoord()),
                    TileId::GetTile(magnification,boundingBox.GetMaxCoord()));

      for (const auto& tileId : box) {
        cellOffsets[tileId.AsPixel()].push_back(offset->second);
      }
    }

    size_t              indexEntries=0;
    size_t              dataSize=1; // Actual data will be prefixed by one empty byte
    std::array<char,10> buffer;

    for (const auto& cell : cellOffsets) {
      indexEntries+=cell.second.size();

      dataSize+=EncodeNumber(cell.second.size(),
                             buffer);

      FileOffset previousOffset=0;
      for (const auto& offset : cell.second) {
        FileOffset data=offset-previousOffset;

        dataSize+=EncodeNumber(data,
                               buffer);

        previousOffset=offset;
      }
    }

    data.dataOffsetBytes=BytesNeededToEncodeNumber(dataSize);

    progress.Info("Writing map for level "+
                  data.optLevel+", index level "+
                  data.indexLevel+", "+
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
    // Everything smaller than 2mm should get dropped. Width, height and DPI come from the Nexus 4
    double        dpi=320.0;
    double        pixel=0.5 /* mm */ * dpi / 25.4 /* inch */;

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

          progress.SetAction("Optimizing type {}",typeConfig.GetTypeInfo(type)->GetName());

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

          for (MagnificationLevel level=parameter.GetOptimizationMinMag();
               level<=parameter.GetOptimizationMaxMag();
               level++) {
            Magnification     magnification(level); // Magnification, we optimize for
            std::list<WayRef> optimizedWays;

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
              progress.Debug("Empty optimization result for level "+level+", no index bitmap generated");

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
                       parameter.GetOptimizationMaxMag())) {
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
