/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/import/GenAreaAreaIndex.h>

#include <vector>

#include <osmscout/TypeFeatures.h>

#include <osmscout/AreaAreaIndex.h>
#include <osmscout/AreaDataFile.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/import/GenOptimizeAreaWayIds.h>

namespace osmscout {

  const char* AreaAreaIndexGenerator::AREAADDRESS_DAT="areaaddress.dat";

  class AreaLocationProcessorFilter : public SortDataGenerator<Area>::ProcessingFilter
  {
  private:
    FileWriter                   writer;
    uint32_t                     overallDataCount;
    NameFeatureValueReader       *nameReader;
    LocationFeatureValueReader   *locationReader;
    AddressFeatureValueReader    *addressReader;
    PostalCodeFeatureValueReader *postalCodeReader;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Area& area,
                 bool& save);
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool AreaLocationProcessorFilter::BeforeProcessingStart(const ImportParameter& parameter,
                                                          Progress& progress,
                                                          const TypeConfig& typeConfig)
  {
    overallDataCount=0;

    nameReader=new NameFeatureValueReader(typeConfig);
    locationReader=new LocationFeatureValueReader(typeConfig);
    addressReader=new AddressFeatureValueReader(typeConfig);
    postalCodeReader= new PostalCodeFeatureValueReader(typeConfig);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AreaAreaIndexGenerator::AREAADDRESS_DAT));

      writer.Write(overallDataCount);
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool AreaLocationProcessorFilter::Process(Progress& progress,
                                            const FileOffset& offset,
                                            Area& area,
                                            bool& /*save*/)
  {
    try {
      for (auto& ring : area.rings) {
        NameFeatureValue       *nameValue=nameReader->GetValue(ring.GetFeatureValueBuffer());
        LocationFeatureValue   *locationValue=locationReader->GetValue(ring.GetFeatureValueBuffer());
        AddressFeatureValue    *addressValue=addressReader->GetValue(ring.GetFeatureValueBuffer());
        PostalCodeFeatureValue *postalCodeValue=postalCodeReader->GetValue(ring.GetFeatureValueBuffer());

        std::string          name;
        std::string          location;
        std::string          address;
        std::string          postalCode;

        if (nameValue!=NULL) {
          name=nameValue->GetName();
        }

        if (locationValue!=NULL &&
            addressValue!=NULL) {
          location=locationValue->GetLocation();
          address=addressValue->GetAddress();
        }

        if (postalCodeValue!=NULL) {
          postalCode=postalCodeValue->GetPostalCode();
        }

        bool isAddress=!ring.GetType()->GetIgnore() &&
                       !location.empty() &&
                       !address.empty();

        bool isPoi=!name.empty() && ring.GetType()->GetIndexAsPOI();

        // We only need location info during import up to this point
        // Thus we delete it now to safe disk space
        if (locationValue!=NULL) {
          size_t locationIndex;

          if (locationReader->GetIndex(ring.GetFeatureValueBuffer(),
                                       locationIndex) &&
              ring.GetFeatureValueBuffer().HasFeature(locationIndex)) {
            ring.UnsetFeature(locationIndex);
          }
        }

        // Same for postal code
        if (postalCodeValue!=NULL) {
          size_t postalCodeIndex;

          if (postalCodeReader->GetIndex(ring.GetFeatureValueBuffer(),
                                         postalCodeIndex) &&
              ring.GetFeatureValueBuffer().HasFeature(postalCodeIndex)) {
            ring.UnsetFeature(postalCodeIndex);
          }
        }

        if (!isAddress && !isPoi) {
          continue;
        }

        if (ring.IsMasterRing() &&
            ring.nodes.empty()) {
          for (const auto& r : area.rings) {
            if (r.IsOuterRing()) {
              writer.WriteFileOffset(offset);
              writer.WriteNumber(ring.GetType()->GetAreaId());

              writer.Write(name);
              writer.Write(postalCode);
              writer.Write(location);
              writer.Write(address);

              writer.Write(r.nodes,false);

              overallDataCount++;
            }
          }
        }
        else {
          writer.WriteFileOffset(offset);
          writer.WriteNumber(ring.GetType()->GetAreaId());

          writer.Write(name);
          writer.Write(postalCode);
          writer.Write(location);
          writer.Write(address);

          writer.Write(ring.nodes,false);

          overallDataCount++;
        }
      }
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool AreaLocationProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                       Progress& progress,
                                                       const TypeConfig& /*typeConfig*/)
  {
    delete nameReader;
    nameReader=NULL;

    delete locationReader;
    locationReader=NULL;

    delete addressReader;
    addressReader=NULL;

    delete postalCodeReader;
    postalCodeReader=NULL;

    try {
      writer.SetPos(0);
      writer.Write(overallDataCount);

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      writer.CloseFailsafe();
      return false;
    }

    return true;
  }

  class AreaNodeReductionProcessorFilter : public SortDataGenerator<Area>::ProcessingFilter
  {
  private:
    std::vector<Point> nodeBuffer;
    size_t             duplicateCount;
    size_t             redundantCount;
    size_t             overallCount;

  private:
    bool IsEqual(const unsigned char buffer1[],
                 const unsigned char buffer2[]);

    bool RemoveDuplicateNodes(Progress& progress,
                              const FileOffset& offset,
                              Area& area,
                              bool& save);

    bool RemoveRedundantNodes(Progress& progress,
                              const FileOffset& offset,
                              Area& area,
                              bool& save);

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);

    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Area& area,
                 bool& save);

    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool AreaNodeReductionProcessorFilter::BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                                               Progress& /*progress*/,
                                                               const TypeConfig& /*typeConfig*/)
  {
    duplicateCount=0;
    redundantCount=0;
    overallCount=0;

    return true;
  }

  bool AreaNodeReductionProcessorFilter::IsEqual(const unsigned char buffer1[],
                                                 const unsigned char buffer2[])
  {
    for (size_t i=0; i<coordByteSize; i++) {
      if (buffer1[i]!=buffer2[i]) {
        return false;
      }
    }

    return true;
  }

  bool AreaNodeReductionProcessorFilter::RemoveDuplicateNodes(Progress& progress,
                                                              const FileOffset& offset,
                                                              Area& area,
                                                              bool& save)
  {
    unsigned char buffers[2][coordByteSize];

    std::vector<Area::Ring>::iterator ring=area.rings.begin();

    while (ring!=area.rings.end()) {
      bool reduced=false;

      if (ring->nodes.size()>=2) {
        size_t lastIndex=0;
        size_t currentIndex=1;

        nodeBuffer.clear();

        ring->GetCoord(0).EncodeToBuffer(buffers[0]);

        nodeBuffer.push_back(ring->nodes[0]);

        for (size_t n=1; n<ring->nodes.size(); n++) {
          ring->GetCoord(n).EncodeToBuffer(buffers[currentIndex]);

          if (IsEqual(buffers[lastIndex],
                      buffers[currentIndex])) {
            if (ring->GetSerial(n)==0) {
              reduced=true;
            }
            else if (ring->GetSerial(n-1)==0) {
              ring->nodes[n-1]=ring->nodes[n];
              reduced=true;
            }
            else if (ring->GetSerial(n-1)==ring->GetSerial(n)) {
              reduced=true;
            }
            else {
              nodeBuffer.push_back(ring->nodes[n]);

              lastIndex=currentIndex;
              currentIndex=(lastIndex+1)%2;
            }
          }
          else {
            nodeBuffer.push_back(ring->nodes[n]);

            lastIndex=currentIndex;
            currentIndex=(lastIndex+1)%2;
          }
        }
      }

      if (reduced) {
        if (nodeBuffer.size()<3) {
          progress.Debug("Area " + std::to_string(offset) + " empty/invalid ring removed after node reduction");
          ring=area.rings.erase(ring);

          if (area.rings.size()==0 ||
              (area.rings.size()==1 &&
               area.rings[0].IsMasterRing())) {
            save=false;
            return true;
          }
        }
        else {
          ring->nodes=nodeBuffer;
          ++ring;
        }
      }
      else {
        ++ring;
      }
    }

    return true;
  }

  bool AreaNodeReductionProcessorFilter::RemoveRedundantNodes(Progress& /*progress*/,
                                                              const FileOffset& /*offset*/,
                                                              Area& area,
                                                              bool& /*save*/)
  {
    for (auto &ring : area.rings) {
      // In this case there is nothing to optimize
      if (ring.nodes.size()<3) {
        continue;
      }

      nodeBuffer.clear();

      size_t last=0;
      size_t current=1;
      bool   reduced=false;

      nodeBuffer.push_back(ring.nodes[0]);

      while (current+1<ring.nodes.size()) {
        double distance=CalculateDistancePointToLineSegment(ring.nodes[current],
                                                            nodeBuffer[last],
                                                            ring.nodes[current+1]);

        if (distance<1/latConversionFactor &&
            ring.GetSerial(current)==0) {
          reduced=true;
          redundantCount++;
          current++;
        }
        else {
          nodeBuffer.push_back(ring.nodes[current]);

          last++;
          current++;
        }
      }

      nodeBuffer.push_back(ring.nodes[current]);

      if (reduced && nodeBuffer.size()<3) {
        reduced=false;
      }

      if (reduced) {
        ring.nodes=nodeBuffer;
      }
    }

    return true;
  }

  bool AreaNodeReductionProcessorFilter::Process(Progress& progress,
                                                 const FileOffset& offset,
                                                 Area& area,
                                                 bool& save)
  {
    for (const auto &ring : area.rings) {
      overallCount+=ring.nodes.size();
    }

    if (!RemoveDuplicateNodes(progress,
                              offset,
                              area,
                              save)) {
      return false;
    }

    if (!save) {
      return true;
    }

    if (!RemoveRedundantNodes(progress,
                              offset,
                              area,
                              save)) {
      return false;
    }

    return true;
  }

  bool AreaNodeReductionProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                            Progress& progress,
                                                            const TypeConfig& /*typeConfig*/)
  {
    progress.Info("Duplicate nodes removed: " + std::to_string(duplicateCount));
    progress.Info("Redundant nodes removed: " + std::to_string(redundantCount));
    progress.Info("Overall nodes: " + std::to_string(overallCount));

    return true;
  }

  class AreaTypeIgnoreProcessorFilter : public SortDataGenerator<Area>::ProcessingFilter
  {
  private:
    uint32_t    removedAreasCount;
    TypeInfoRef typeInfoIgnore;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Area& area,
                 bool& save);
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool AreaTypeIgnoreProcessorFilter::BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                                            Progress& /*progress*/,
                                                            const TypeConfig& typeConfig)
  {
    removedAreasCount=0;
    typeInfoIgnore=typeConfig.typeInfoIgnore;

    return true;
  }

  bool AreaTypeIgnoreProcessorFilter::Process(Progress& /*progress*/,
                                              const FileOffset& /*offset*/,
                                              Area& area,
                                              bool& save)
  {
    save=area.GetType()!=NULL &&
         area.GetType()!=typeInfoIgnore;

    if (!save) {
      removedAreasCount++;
    }

    return true;
  }

  bool AreaTypeIgnoreProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                         Progress& progress,
                                                         const TypeConfig& /*typeConfig*/)
  {
    progress.Info("Areas without a type removed: " + std::to_string(removedAreasCount));

    return true;
  }

  void AreaAreaIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                             ImportModuleDescription& description) const
  {
    description.SetName("AreaAreaIndexGenerator");
    description.SetDescription("Index areas for area lookup");

    description.AddRequiredFile(OptimizeAreaWayIdsGenerator::AREAS3_TMP);

    description.AddProvidedFile(AreaAreaIndex::AREA_AREA_IDX);
    description.AddProvidedFile(AreaDataFile::AREAS_DAT);

    description.AddProvidedDebuggingFile(AreaAreaIndexGenerator::AREAADDRESS_DAT);
    description.AddProvidedDebuggingFile(AreaDataFile::AREAS_IDMAP);
  }

  size_t AreaAreaIndexGenerator::CalculateLevel(const ImportParameter& parameter,
                                                const GeoBox& boundingBox) const
  {
    size_t indexLevel=parameter.GetAreaAreaIndexMaxMag();

    while (true) {
      if (boundingBox.GetWidth()<=cellDimension[indexLevel].width &&
          boundingBox.GetHeight()<=cellDimension[indexLevel].height) {
        break;
      }

      if (indexLevel==0) {
        break;
      }

      indexLevel--;
    }

    return indexLevel;
  }

  /**
   * Assure that there is a parent cell in the parent level for
   * each cell in a given level.
   */
  void AreaAreaIndexGenerator::EnrichLevels(std::vector<Level>& levels)
  {
    for (size_t l=0; l<levels.size()-1; l++) {
      size_t level=levels.size()-l-1;

      for (const auto& cellEntry : levels[level]) {
        Pixel parentCell(cellEntry.first.x/2,cellEntry.first.y/2);

        if (levels[level-1].find(parentCell)==levels[level-1].end()) {
          levels[level-1].insert(std::make_pair(parentCell,AreaLeaf()));
        }
      }
    }
  }

  bool AreaAreaIndexGenerator::CopyData(const TypeConfig& typeConfig,
                                        Progress& progress,
                                        FileScanner& scanner,
                                        FileWriter& dataWriter,
                                        FileWriter& mapWriter,
                                        const std::list<FileOffset>& srcOffsets,
                                        FileOffset& dataStartOffset,
                                        uint32_t& dataWrittenCount)
  {
    dataStartOffset=0;

    for (FileOffset srcOffset : srcOffsets) {
      uint8_t    objectType;
      Id         id;
      Area       area;
      FileOffset dstOffset;
      bool       save=true;

      scanner.SetPos(srcOffset);

      scanner.Read(objectType);
      scanner.Read(id);

      area.Read(typeConfig,
                scanner);

      //  std::cout << (size_t)objectType << " " << id << " " << area.GetType()->GetName() << " " << area.GetType()->GetIndex() << std::endl;

      dstOffset=dataWriter.GetPos();

      for (const auto& filter : filters) {
        if (!filter->Process(progress,
                             dstOffset,
                             area,
                             save)) {
          progress.Error(std::string("Error while processing data entry to file '")+
                         dataWriter.GetFilename()+"'");

          return false;
        }

        if (!save) {
          break;
        }
      }

      if (!save) {
        continue;
      }

      if (dataStartOffset==0) {
        dataStartOffset=dstOffset;
      }

      area.Write(typeConfig,
                 dataWriter);

      mapWriter.Write(id);
      mapWriter.Write(objectType);
      mapWriter.WriteFileOffset(dstOffset);

      dataWrittenCount++;
    }

    return true;
  }

  bool AreaAreaIndexGenerator::WriteChildCells(const TypeConfig& typeConfig,
                                               Progress& progress,
                                               const ImportParameter& parameter,
                                               FileScanner& scanner,
                                               FileWriter& indexWriter,
                                               FileWriter& dataWriter,
                                               FileWriter& mapWriter,
                                               const std::vector<Level>& levels,
                                               size_t level,
                                               const Pixel& pixel,
                                               FileOffset& offset,
                                               uint32_t& dataWrittenCount)
  {
    Pixel      topLeftPixel(pixel.x*2,pixel.y*2+1);
    FileOffset topLeftOffset=0;

    auto topLeftCell=levels[level+1].find(topLeftPixel);

    if (topLeftCell!=levels[level+1].end()) {
      if (!WriteCell(typeConfig,
                     progress,
                     parameter,
                     scanner,
                     indexWriter,
                     dataWriter,
                     mapWriter,
                     levels,
                     level+1,
                     topLeftPixel,
                     topLeftCell->second,
                     topLeftOffset,
                     dataWrittenCount)) {
        return false;
      }
    }

    Pixel      topRightPixel(pixel.x*2+1,pixel.y*2+1);
    FileOffset topRightOffset=0;

    auto topRightCell=levels[level+1].find(topRightPixel);

    if (topRightCell!=levels[level+1].end()) {
      if (!WriteCell(typeConfig,
                     progress,
                     parameter,
                     scanner,
                     indexWriter,
                     dataWriter,
                     mapWriter,
                     levels,
                     level+1,
                     topRightPixel,
                     topRightCell->second,
                     topRightOffset,
                     dataWrittenCount)) {
        return false;
      }
    }

    Pixel      bottomLeftPixel(pixel.x*2,pixel.y*2);
    FileOffset bottomLeftOffset=0;

    auto bottomLeftCell=levels[level+1].find(bottomLeftPixel);

    if (bottomLeftCell!=levels[level+1].end()) {
      if (!WriteCell(typeConfig,
                     progress,
                     parameter,
                     scanner,
                     indexWriter,
                     dataWriter,
                     mapWriter,
                     levels,
                     level+1,
                     bottomLeftPixel,
                     bottomLeftCell->second,
                     bottomLeftOffset,
                     dataWrittenCount)) {
        return false;
      }
    }

    Pixel      bottomRightPixel(pixel.x*2+1,pixel.y*2);
    FileOffset bottomRightOffset=0;

    auto bottomRightCell=levels[level+1].find(bottomRightPixel);

    if (bottomRightCell!=levels[level+1].end()) {
      if (!WriteCell(typeConfig,
                     progress,
                     parameter,
                     scanner,
                     indexWriter,
                     dataWriter,
                     mapWriter,
                     levels,
                     level+1,
                     bottomRightPixel,
                     bottomRightCell->second,
                     bottomRightOffset,
                     dataWrittenCount)) {
        return false;
      }
    }

    offset=indexWriter.GetPos();

    if (topLeftOffset!=0) {
      topLeftOffset=offset-topLeftOffset;
    }

    indexWriter.WriteNumber(topLeftOffset);

    if (topRightOffset!=0) {
      topRightOffset=offset-topRightOffset;
    }

    indexWriter.WriteNumber(topRightOffset);

    if (bottomLeftOffset!=0) {
      bottomLeftOffset=offset-bottomLeftOffset;
    }

    indexWriter.WriteNumber(bottomLeftOffset);

    if (bottomRightOffset!=0) {
      bottomRightOffset=offset-bottomRightOffset;
    }

    indexWriter.WriteNumber(bottomRightOffset);

    return true;
  }

  bool AreaAreaIndexGenerator::WriteCell(const TypeConfig& typeConfig,
                                         Progress& progress,
                                         const ImportParameter& parameter,
                                         FileScanner& scanner,
                                         FileWriter& indexWriter,
                                         FileWriter& dataWriter,
                                         FileWriter& mapWriter,
                                         const std::vector<Level>& levels,
                                         size_t level,
                                         const Pixel& pixel,
                                         const AreaLeaf& leaf,
                                         FileOffset& dataStartOffset,
                                         uint32_t& dataWrittenCount)
  {
    if (level<parameter.GetAreaAreaIndexMaxMag()) {
      if (!WriteChildCells(typeConfig,
                           progress,
                           parameter,
                           scanner,
                           indexWriter,
                           dataWriter,
                           mapWriter,
                           levels,
                           level,
                           pixel,
                           dataStartOffset,
                           dataWrittenCount)) {
        return false;
      }
    }
    else {
      dataStartOffset=indexWriter.GetPos();
    }

    std::unordered_map<TypeId,std::list<FileOffset>> offsetsTypeMap;

    for (const auto& entry : leaf.areas) {
      offsetsTypeMap[entry.type].push_back(entry.offset);
    }

    // Number of types
    indexWriter.WriteNumber((uint32_t)offsetsTypeMap.size());

    FileOffset prevObjectStartOffset=0;

    for (const auto& entry : offsetsTypeMap) {
      FileOffset objectStartOffset=0;

      // Note, that if we optimize everything away, we are left here with objectStartOffset==0
      // The index reading code has to handle this!
      CopyData(typeConfig,
               progress,
               scanner,
               dataWriter,
               mapWriter,
               entry.second,
               objectStartOffset,
               dataWrittenCount);

      indexWriter.WriteTypeId(entry.first,typeConfig.GetAreaTypeIdBytes());
      indexWriter.WriteNumber((uint32_t)entry.second.size());
      indexWriter.WriteNumber(objectStartOffset-prevObjectStartOffset);

      prevObjectStartOffset=objectStartOffset;
    }

    return !indexWriter.HasError();
  }

  bool AreaAreaIndexGenerator::BuildInMemoryIndex(const TypeConfigRef& typeConfig,
                                                  const ImportParameter& parameter,
                                                  Progress& progress,
                                                  FileScanner& scanner,
                                                  std::vector<Level>& levels)
  {
    uint32_t areaCount=0;

    scanner.GotoBegin();

    scanner.Read(areaCount);

    for (uint32_t a=1; a<=areaCount; a++) {
      uint8_t    objectType;
      Id         id;
      FileOffset offset;
      Area       area;

      progress.SetProgress(a,areaCount);

      offset=scanner.GetPos();

      scanner.Read(objectType),
      scanner.Read(id);

      area.Read(*typeConfig,scanner);

      GeoBox boundingBox;

      area.GetBoundingBox(boundingBox);

      GeoCoord center=boundingBox.GetCenter();

      //
      // Calculate highest level where the bounding box completely
      // fits in the cell size and assign area to the tiles that
      // hold the geometric center of the tile.
      //

      size_t level=CalculateLevel(parameter,boundingBox);

      // Calculate index of tile that contains the geometric center of the area
      uint32_t x=(uint32_t)((center.GetLon()+180.0)/cellDimension[level].width);
      uint32_t y=(uint32_t)((center.GetLat()+90.0)/cellDimension[level].height);

      Entry entry;

      entry.type=area.GetType()->GetAreaId();
      entry.offset=offset;

      levels[level][Pixel(x,y)].areas.push_back(entry);
    }

    return true;
  }

  bool AreaAreaIndexGenerator::ImportInternal(const TypeConfigRef& typeConfig,
                                              const ImportParameter& parameter,
                                              Progress& progress)
  {
    FileScanner         scanner;
    FileWriter          indexWriter;
    FileWriter          dataWriter;
    FileWriter          mapWriter;
    std::vector<Level>  levels;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   OptimizeAreaWayIdsGenerator::AREAS3_TMP),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      progress.SetAction("Building in memory area index from '"+scanner.GetFilename()+"'");

      levels.resize(parameter.GetAreaAreaIndexMaxMag()+1);

      if (!BuildInMemoryIndex(typeConfig,
                              parameter,
                              progress,
                              scanner,
                              levels)) {
        return false;
      }

      progress.SetAction("Enriching index tree");

      EnrichLevels(levels);

      //assert(levels[0].size()==1);

      for (size_t i=0; i<levels.size(); i++) {
        progress.Info("Level "+std::to_string(i)+" has " + std::to_string(levels[i].size())+" entries");
      }

      //
      // Writing index, data and idmap files
      //

      uint32_t    overallDataCount=0;

      FileOffset topLevelOffset=0;
      FileOffset topLevelOffsetOffset; // Offset of the top level entry

      // Index file

      indexWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       AreaAreaIndex::AREA_AREA_IDX));

      indexWriter.WriteNumber((uint32_t)parameter.GetAreaAreaIndexMaxMag()); // MaxMag

      topLevelOffsetOffset=indexWriter.GetPos();

      // This is not the final value, that will be written later on
      indexWriter.WriteFileOffset(topLevelOffset);

      // Data file

      dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      AreaDataFile::AREAS_DAT));

      dataWriter.Write(overallDataCount);

      // Id map file

      mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                          AreaDataFile::AREAS_IDMAP));

      mapWriter.Write(overallDataCount);

      progress.SetAction("Writing files '"+indexWriter.GetFilename()+"', '"+dataWriter.GetFilename()+"' and '"+
                         mapWriter.GetFilename()+"'");

      if (!WriteCell(*typeConfig,
                     progress,
                     parameter,
                     scanner,
                     indexWriter,
                     dataWriter,
                     mapWriter,
                     levels,
                     0,
                     Pixel(0,0),
                     levels[0][Pixel(0,0)],
                     topLevelOffset,
                     overallDataCount)) {
        return false;
      }

      // Finishing index file

      indexWriter.SetPos(topLevelOffsetOffset);
      indexWriter.WriteFileOffset(topLevelOffset);

      // Finishing data file

      dataWriter.SetPos(0);
      dataWriter.Write(overallDataCount);

      // Finishing id map file

      mapWriter.SetPos(0);
      mapWriter.Write(overallDataCount);

      progress.Info(std::to_string(overallDataCount) + " object(s) written to file '"+dataWriter.GetFilename()+"'");

      scanner.Close();
      indexWriter.Close();
      dataWriter.Close();
      mapWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      indexWriter.CloseFailsafe();
      dataWriter.CloseFailsafe();
      mapWriter.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool AreaAreaIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {

    try {
      for (auto& filter : filters) {
        if (!filter->BeforeProcessingStart(parameter,
                                           progress,
                                           *typeConfig)) {
          progress.Error("Cannot initialize processor filter");

          return false;
        }
      }
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

      bool result=ImportInternal(typeConfig,
                                 parameter,
                                 progress);

    try {
      for (auto& filter : filters) {
        if (!filter->AfterProcessingEnd(parameter,
                                        progress,
                                        *typeConfig)) {
          progress.Error("Cannot deinitialize processor filter");
          return false;
        }
      }
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

    return result;
  }

  AreaAreaIndexGenerator::AreaAreaIndexGenerator()
  {
    filters.push_back(std::make_shared<AreaLocationProcessorFilter>());
    filters.push_back(std::make_shared<AreaNodeReductionProcessorFilter>());
    filters.push_back(std::make_shared<AreaTypeIgnoreProcessorFilter>());
  }
}
