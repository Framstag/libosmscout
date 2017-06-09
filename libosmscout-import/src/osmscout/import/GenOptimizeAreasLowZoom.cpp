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

#include <osmscout/import/GenOptimizeAreasLowZoom.h>

#include <osmscout/Pixel.h>
#include <osmscout/Way.h>

#include <osmscout/OptimizeAreasLowZoom.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>
#include <osmscout/AreaDataFile.h>

namespace osmscout
{
  OptimizeAreasLowZoomGenerator::TypeData::TypeData()
  : optLevel(0),
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

  void OptimizeAreasLowZoomGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                     ImportModuleDescription& description) const
  {
    description.SetName("OptimizeAreasLowZoomGenerator");
    description.SetDescription("Create index for area lookup of reduced resolution areas");

    description.AddRequiredFile(AreaDataFile::AREAS_DAT);

    description.AddProvidedOptionalFile(OptimizeAreasLowZoom::FILE_AREASOPT_DAT);
  }

  void OptimizeAreasLowZoomGenerator::GetAreaTypesToOptimize(const TypeConfig& typeConfig,
                                                             TypeInfoSet& types)
  {
    types.Clear();

    for (auto &type : typeConfig.GetAreaTypes()) {
      if (!type->GetIgnore() &&
          !type->IsInternal() &&
          type->GetOptimizeLowZoom()) {
        types.Set(type);
      }
    }
  }

  bool OptimizeAreasLowZoomGenerator::WriteTypeData(FileWriter& writer,
                                                    const TypeData& data)
  {
    assert(data.type);

    writer.Write(data.type->GetAreaId());
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

  bool OptimizeAreasLowZoomGenerator::WriteHeader(FileWriter& writer,
                                                  const std::list<TypeData>& areaTypesData,
                                                  uint32_t optimizeMaxMap)
  {
    writer.Write(optimizeMaxMap);
    writer.Write((uint32_t)areaTypesData.size());

    for (const auto &typeData : areaTypesData) {
      if (!WriteTypeData(writer,
                         typeData)) {
        return false;
      }
    }

    return true;
  }

  bool OptimizeAreasLowZoomGenerator::GetAreas(const TypeConfig& typeConfig,
                                               const ImportParameter& parameter,
                                               Progress& progress,
                                               FileScanner& scanner,
                                               const TypeInfoSet& types,
                                               std::vector<std::list<AreaRef> >& areas,
                                               TypeInfoSet& loadedTypes)
  {
    uint32_t    areaCount=0;
    size_t      collectedAreasCount=0;

    loadedTypes=types;

    progress.SetAction("Collecting area data to optimize");

    scanner.GotoBegin();

    scanner.Read(areaCount);

    for (uint32_t a=1; a<=areaCount; a++) {
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,areaCount);

      area->Read(typeConfig,
                 scanner);

      if (loadedTypes.IsSet(area->GetType())) {
        areas[area->GetType()->GetIndex()].push_back(area);

        collectedAreasCount++;

        while (collectedAreasCount>parameter.GetOptimizationMaxWayCount() &&
               loadedTypes.Size()>1) {
          TypeInfoRef victimType;

          for (auto &type : loadedTypes) {
            if (areas[type->GetIndex()].size()>0 &&
                (!victimType ||
                 areas[type->GetIndex()].size()<areas[victimType->GetIndex()].size())) {
              victimType=type;
            }
          }

          assert(victimType);

          collectedAreasCount-=areas[victimType->GetIndex()].size();
          areas[victimType->GetIndex()].clear();
          loadedTypes.Remove(victimType);
        }
      }
    }

    progress.Info("Collected "+NumberToString(collectedAreasCount)+" areas for "+NumberToString(loadedTypes.Size())+" types");

    return !scanner.HasError();
  }

  void OptimizeAreasLowZoomGenerator::OptimizeAreas(const std::list<AreaRef>& areas,
                                                    std::list<AreaRef>& optimizedAreas,
                                                    size_t width,
                                                    size_t height,
                                                    double dpi,
                                                    double pixel,
                                                    const Magnification& magnification,
                                                    TransPolygon::OptimizeMethod optimizeAreaMethod)
  {
    MercatorProjection projection;

    projection.Set(GeoCoord(0.0,0.0),magnification,dpi,width,height);

    for (auto &area :areas) {
      TransPolygon            polygon;
      std::vector<Area::Ring> newRings;
      double                  xmin;
      double                  xmax;
      double                  ymin;
      double                  ymax;

      size_t r=0;
      while (r<area->rings.size()) {
        if (!(area->rings[r].IsMasterRing() &&
              area->rings[r].nodes.empty())) {
          polygon.TransformArea(projection,
                                optimizeAreaMethod,
                                area->rings[r].nodes,
                                pixel/8.0);

          polygon.GetBoundingBox(xmin,ymin,xmax,ymax);

          if (polygon.IsEmpty() ||
              (xmax-xmin<=pixel &&
               ymax-ymin<=pixel)) {
            // We drop all sub roles of the current role, too
            size_t s=r;

            while (s+1<area->rings.size() &&
                   area->rings[s+1].GetRing()>area->rings[r].GetRing()) {
              s++;
            }

            r=s+1;
            continue;
          }
        }

        newRings.push_back(area->rings[r]);

        if (!(area->rings[r].IsMasterRing() &&
              area->rings[r].nodes.empty())) {
          newRings.back().nodes.clear();

          for (size_t i=polygon.GetStart();
               i<=polygon.GetEnd();
               i++) {
            if (polygon.points[i].draw) {
              newRings.back().nodes.push_back(area->rings[r].nodes[i]);
            }
          }
        }

        r++;
      }

      // MAster ring can have nodes, but does not need to have
      if (area->rings.front().IsMasterRing()) {
        if (area->rings.front().nodes.empty()) {
          if (newRings.size()==1) {
            // Master ring is empty and the only one left => skip
            continue;
          }
        }
        else {
          if (newRings.size()==0) {
            // Master ring is not empty and all rings were dropped => skip
            continue;
          }
        }
      }
      else {
        if (newRings.size()==0) {
          // No master ring is and all rings were dropped => skip
          continue;
        }
      }

      AreaRef copiedArea=std::make_shared<Area>(*area);

      copiedArea->rings=newRings;

      optimizedAreas.push_back(copiedArea);
    }
  }


  void OptimizeAreasLowZoomGenerator::GetAreaIndexLevel(const ImportParameter& parameter,
                                                        const std::list<AreaRef>& areas,
                                                        TypeData& typeData)
  {
    size_t level=5;//parameter.GetOptimizationMinMag();

    while (true) {
      std::map<Pixel,size_t> cellFillCount;

      for (const auto& area : areas) {
        GeoBox  boundingBox;

        area->GetBoundingBox(boundingBox);

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

  void OptimizeAreasLowZoomGenerator::WriteAreas(const TypeConfig& typeConfig,
                                                 FileWriter& writer,
                                                 const std::list<AreaRef>& areas,
                                                 FileOffsetFileOffsetMap& offsets)
  {
    for (const auto &area : areas) {
      offsets[area->GetFileOffset()]=writer.GetPos();

      area->WriteOptimized(typeConfig,
                           writer);
    }
  }

  bool OptimizeAreasLowZoomGenerator::WriteAreaBitmap(Progress& progress,
                                                      FileWriter& writer,
                                                      const std::list<AreaRef>& areas,
                                                      const FileOffsetFileOffsetMap& offsets,
                                                      TypeData& data)
  {
    // We do not write a bitmap, if there is not data to map
    if (areas.empty()) {
      return true;
    }

    double                                 cellWidth=cellDimension[data.indexLevel].width;
    double                                 cellHeight=cellDimension[data.indexLevel].height;
    std::map<Pixel,std::list<FileOffset> > cellOffsets;

    for (const auto& area : areas) {
      GeoBox                                  boundingBox;
      FileOffsetFileOffsetMap::const_iterator offset=offsets.find(area->GetFileOffset());

      if (offset==offsets.end()) {
        continue;
      }

      area->GetBoundingBox(boundingBox);

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
    size_t dataSize=1;  // Actual data will be prefixed by one empty byte
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
                  NumberToString(data.optLevel)+", index level "+
                  NumberToString(data.indexLevel)+", "+
                  NumberToString(cellOffsets.size())+" cells, "+
                  NumberToString(indexEntries)+" entries, "+
                  ByteSizeToString(1.0*data.cellXCount*data.cellYCount*data.dataOffsetBytes+dataSize));

    data.bitmapOffset=writer.GetPos();

    // Write the bitmap with offsets for each cell
    // We prefill with zero and only overrite cells that have data
    // So zero means "no data for this cell"
    for (size_t i=0; i<data.cellXCount*data.cellYCount; i++) {
      writer.WriteFileOffset((FileOffset)0,
                             data.dataOffsetBytes);
    }

    FileOffset dataStartOffset;
    dataStartOffset=writer.GetPos();

    // Move data start by one byte. It creates little bit larger output file.
    // But without it 0 is valid cell offset and these data will not be visible,
    // because for reader means that this cell has no data!

    // TODO: when data format will be changing, consider usage ones (0xFF..FF) as empty placeholder
    writer.WriteFileOffset((FileOffset)0, 1);

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

    return true;
  }

  bool OptimizeAreasLowZoomGenerator::HandleAreas(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const TypeConfig& typeConfig,
                                                  FileWriter& writer,
                                                  const TypeInfoSet& types,
                                                  std::list<TypeData>& typesData)
  {
    FileScanner scanner;
    // Everything smaller than 2mm should get dropped. Width, height and DPI come from the Nexus 4
    double dpi=320.0;
    double pixel=2.0/* mm */ * dpi / 25.4 /* inch */;

    progress.Info("Minimum visible size in pixel: "+NumberToString((unsigned long)pixel));

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      TypeInfoSet                      typesToProcess(types);
      std::vector<std::list<AreaRef> > allAreas(typeConfig.GetTypeCount());

      while (true) {
        //
        // Load type data
        //

        TypeInfoSet loadedTypes;

        if (!GetAreas(typeConfig,
                      parameter,
                      progress,
                      scanner,
                      typesToProcess,
                      allAreas,
                      loadedTypes)) {
          return false;
        }

        typesToProcess.Remove(loadedTypes);

        for (const auto& type : loadedTypes) {
          progress.SetAction("Optimizing type "+ type->GetName());

          for (uint32_t level=parameter.GetOptimizationMinMag();
               level<=parameter.GetOptimizationMaxMag();
               level++) {
            Magnification      magnification; // Magnification, we optimize for
            std::list<AreaRef> optimizedAreas;

            magnification.SetLevel(level);

            OptimizeAreas(allAreas[type->GetIndex()],
                          optimizedAreas,
                          800,480,
                          dpi,
                          pixel,
                          magnification,
                          parameter.GetOptimizationWayMethod());

            if (optimizedAreas.empty()) {
              progress.Debug("Empty optimization result for level "+NumberToString(level)+", no index generated");

              TypeData typeData;

              typeData.type=type;
              typeData.optLevel=level;

              typesData.push_back(typeData);

              continue;
            }

            progress.Info("Optimized from "+NumberToString(allAreas[type->GetIndex()].size())+" to "+NumberToString(optimizedAreas.size())+" areas");

            /*
            size_t optAreas=optimizedAreas.size();
            size_t optRoles=0;
            size_t optNodes=0;

            for (std::list<AreaRef>::const_iterator a=optimizedAreas.begin();
                a!=optimizedAreas.end();
                ++a) {
              AreaRef area=*a;

              optRoles+=area->rings.size();

              for (size_t r=0; r<area->rings.size(); r++) {
                optNodes+=area->rings[r].nodes.size();
              }
            }*/

            /*
            std::cout << "Areas: " << origAreas << " => " << optAreas << std::endl;
            std::cout << "Roles: " << origRoles << " => " << optRoles << std::endl;
            std::cout << "Nodes: " << origNodes << " => " << optNodes << std::endl;*/

            TypeData typeData;

            typeData.type=type;
            typeData.optLevel=level;

            GetAreaIndexLevel(parameter,
                              optimizedAreas,
                              typeData);

            //std::cout << "Resulting index level: " << typeData.indexLevel << ", " << typeData.indexCells << ", " << typeData.indexEntries << std::endl;

            FileOffsetFileOffsetMap offsets;

            WriteAreas(typeConfig,
                       writer,
                       optimizedAreas,
                       offsets);

            if (!WriteAreaBitmap(progress,
                                 writer,
                                 optimizedAreas,
                                 offsets,
                                 typeData)) {
              return false;
            }

            typesData.push_back(typeData);
          }

          allAreas[type->GetIndex()].clear();
        }

        if (typesToProcess.Empty()) {
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

  bool OptimizeAreasLowZoomGenerator::Import(const TypeConfigRef& typeConfig,
                                             const ImportParameter& parameter,
                                             Progress& progress)
  {
    FileOffset          indexOffset=0;
    FileWriter          writer;
    Magnification       magnification; // Magnification, we optimize for
    TypeInfoSet         areaTypes;     // Types we optimize
    std::list<TypeData> areaTypesData;

    GetAreaTypesToOptimize(*typeConfig,
                           areaTypes);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  OptimizeAreasLowZoom::FILE_AREASOPT_DAT));

      //
      // Write header
      //

      writer.WriteFileOffset(indexOffset);

      if (!HandleAreas(parameter,
                       progress,
                       *typeConfig,
                       writer,
                       areaTypes,
                       areaTypesData)) {
        progress.Error("Error while optimizing areas");
        return false;
      }

      // Position of the index
      indexOffset=writer.GetPos();

      if (!WriteHeader(writer,
                       areaTypesData,
                       (uint32_t)parameter.GetOptimizationMaxMag())) {
        progress.Error("Cannot write file header");
        return false;
      }

      writer.GotoBegin();
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
