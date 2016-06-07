/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/import/SortWayDat.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/WayDataFile.h>

#include <osmscout/util/Geometry.h>

#include <osmscout/import/GenWayWayDat.h>
#include <osmscout/import/GenOptimizeAreaWayIds.h>

namespace osmscout {

  const char* SortWayDataGenerator::WAYADDRESS_DAT="wayaddress.dat";

  class WayLocationProcessorFilter : public SortDataGenerator<Way>::ProcessingFilter
  {
  private:
    FileWriter                 writer;
    uint32_t                   overallDataCount;
    NameFeatureValueReader     *nameReader;
    LocationFeatureValueReader *locationReader;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Way& way,
                 bool& save);
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool WayLocationProcessorFilter::BeforeProcessingStart(const ImportParameter& parameter,
                                                         Progress& progress,
                                                         const TypeConfig& typeConfig)
  {
    overallDataCount=0;

    nameReader=new NameFeatureValueReader(typeConfig);
    locationReader=new LocationFeatureValueReader(typeConfig);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  SortWayDataGenerator::WAYADDRESS_DAT));

      writer.Write(overallDataCount);
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

    return true;
  }

  bool WayLocationProcessorFilter::Process(Progress& progress,
                                           const FileOffset& offset,
                                           Way& way,
                                           bool& /*save*/)
  {
    try {
      if (!way.GetType()->GetIndexAsPOI()) {
        return true;
      }

      NameFeatureValue     *nameValue=nameReader->GetValue(way.GetFeatureValueBuffer());

      if (nameValue==NULL) {
        return true;
      }

      LocationFeatureValue *locationValue=locationReader->GetValue(way.GetFeatureValueBuffer());
      std::string          name;
      std::string          location;
      std::string          address;

      name=nameValue->GetName();

      if (locationValue!=NULL) {
        location=locationValue->GetLocation();
      }

      writer.WriteFileOffset(offset);
      writer.WriteNumber(way.GetType()->GetWayId());
      writer.Write(name);
      writer.Write(location);
      writer.Write(way.GetNodes(),false);

      overallDataCount++;
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

    return true;
  }

  bool WayLocationProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                      Progress& progress,
                                                      const TypeConfig& /*typeConfig*/)
  {
    delete nameReader;
    nameReader=NULL;

    delete locationReader;
    locationReader=NULL;

    writer.SetPos(0);
    writer.Write(overallDataCount);

    try {
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  class WayNodeReductionProcessorFilter : public SortDataGenerator<Way>::ProcessingFilter
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
                              Way& way,
                              bool& save);

    bool RemoveRedundantNodes(Progress& progress,
                              const FileOffset& offset,
                              Way& way,
                              bool& save);

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);

    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Way& way,
                 bool& save);

    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool WayNodeReductionProcessorFilter::BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                                              Progress& /*progress*/,
                                                              const TypeConfig& /*typeConfig*/)
  {
    duplicateCount=0;
    redundantCount=0;
    overallCount=0;

    return true;
  }

  bool WayNodeReductionProcessorFilter::IsEqual(const unsigned char buffer1[],
                                                const unsigned char buffer2[])
  {
    for (size_t i=0; i<coordByteSize; i++) {
      if (buffer1[i]!=buffer2[i]) {
        return false;
      }
    }

    return true;
  }

  bool WayNodeReductionProcessorFilter::RemoveDuplicateNodes(Progress& progress,
                                                             const FileOffset& offset,
                                                             Way& way,
                                                             bool& save)
  {
    unsigned char buffers[2][coordByteSize];

    bool reduced=false;

    if (way.GetNodes().size()>=2) {
      size_t lastIndex=0;
      size_t currentIndex=1;

      nodeBuffer.clear();

      // Prefill with the first coordinate
      way.GetNodes()[0].GetCoord().EncodeToBuffer(buffers[0]);

      nodeBuffer.push_back(way.GetNodes()[0]);

      for (size_t n=1; n<way.GetNodes().size(); n++) {
        way.GetNodes()[n].GetCoord().EncodeToBuffer(buffers[currentIndex]);

        if (IsEqual(buffers[lastIndex],
                    buffers[currentIndex])) {
          if (way.GetSerial(n)==0) {
            duplicateCount++;
            reduced=true;
          }
          else if (way.GetSerial(n-1)==0) {
            way.MutableNodes()[n-1]=way.GetNodes()[n];
            duplicateCount++;
            reduced=true;
          }
          else {
            nodeBuffer.push_back(way.GetNodes()[n]);

            lastIndex=currentIndex;
            currentIndex=(lastIndex+1)%2;
          }
        }
        else {
          nodeBuffer.push_back(way.GetNodes()[n]);

          lastIndex=currentIndex;
          currentIndex=(lastIndex+1)%2;
        }
      }
    }

    if (reduced) {
      if (nodeBuffer.size()<2) {
        progress.Debug("Way " + NumberToString(offset) + " empty/invalid after node reduction");
        save=false;
        return true;
      }
      else {
        way.SetNodes(nodeBuffer);
      }
    }

    return true;
  }

  bool WayNodeReductionProcessorFilter::RemoveRedundantNodes(Progress& /*progress*/,
                                                             const FileOffset& /*offset*/,
                                                             Way& way,
                                                             bool& /*save*/)
  {
    // In this case there is nothing to optimize
    if (way.GetNodes().size()<3) {
      return true;
    }

    nodeBuffer.clear();

    size_t last=0;
    size_t current=1;
    bool   reduced=false;

    nodeBuffer.push_back(way.GetNodes()[0]);

    while (current+1<way.GetNodes().size()) {
      double distance=CalculateDistancePointToLineSegment(way.GetNodes()[current],
                                                          nodeBuffer[last],
                                                          way.GetNodes()[current+1]);

      if (distance<1/latConversionFactor &&
          way.GetSerial(current)==0) {
        reduced=true;
        redundantCount++;
        current++;
      }
      else {
        nodeBuffer.push_back(way.GetNodes()[current]);

        last++;
        current++;
      }
    }

    nodeBuffer.push_back(way.GetNodes()[current]);

    if (reduced) {
      way.SetNodes(nodeBuffer);
    }

    return true;
  }

  bool WayNodeReductionProcessorFilter::Process(Progress& progress,
                                                const FileOffset& offset,
                                                Way& way,
                                                bool& save)
  {
    overallCount+=way.GetNodes().size();

    if (!RemoveDuplicateNodes(progress,
                              offset,
                              way,
                              save)) {
      return false;
    }

    if (!RemoveRedundantNodes(progress,
                              offset,
                              way,
                              save)) {
      return false;
    }

    return true;
  }

  bool WayNodeReductionProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                           Progress& progress,
                                                           const TypeConfig& /*typeConfig*/)
  {
    progress.Info("Duplicate nodes removed: " + NumberToString(duplicateCount));
    progress.Info("Redundant nodes removed: " + NumberToString(redundantCount));
    progress.Info("Overall nodes: " + NumberToString(overallCount));

    return true;
  }

  class WayTypeIgnoreProcessorFilter : public SortDataGenerator<Way>::ProcessingFilter
  {
  private:
    uint32_t    removedWaysCount;
    TypeInfoRef typeInfoIgnore;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Way& way,
                 bool& save);
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig);
  };

  bool WayTypeIgnoreProcessorFilter::BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                                           Progress& /*progress*/,
                                                           const TypeConfig& typeConfig)
  {
    removedWaysCount=0;
    typeInfoIgnore=typeConfig.typeInfoIgnore;

    return true;
  }

  bool WayTypeIgnoreProcessorFilter::Process(Progress& /*progress*/,
                                             const FileOffset& /*offset*/,
                                             Way& way,
                                             bool& save)
  {
    save=way.GetType()!=NULL &&
         way.GetType()!=typeInfoIgnore;

    if (!save) {
      removedWaysCount++;
    }

    return true;
  }

  bool WayTypeIgnoreProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                        Progress& progress,
                                                        const TypeConfig& /*typeConfig*/)
  {
    progress.Info("Ways without a type removed: " + NumberToString(removedWaysCount));

    return true;
  }

  void SortWayDataGenerator::GetTopLeftCoordinate(const Way& data,
                                                  GeoCoord& coord)
  {
    coord=data.GetNodes()[0].GetCoord();

    for (size_t n=1; n<data.GetNodes().size(); n++) {
      coord.Set(std::max(coord.GetLat(),data.GetNodes()[n].GetLat()),
                std::min(coord.GetLon(),data.GetNodes()[n].GetLon()));
    }
  }

  SortWayDataGenerator::SortWayDataGenerator()
  : SortDataGenerator<Way>(WayDataFile::WAYS_DAT,WayDataFile::WAYS_IDMAP)
  {
    AddSource(OptimizeAreaWayIdsGenerator::WAYS_TMP);

    AddFilter(std::make_shared<WayLocationProcessorFilter>());
    AddFilter(std::make_shared<WayNodeReductionProcessorFilter>());
    AddFilter(std::make_shared<WayTypeIgnoreProcessorFilter>());
  }

  void SortWayDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                             ImportModuleDescription& description) const
  {
    description.SetName("SortWayDataGenerator");
    description.SetDescription("Sort ways to improve lookup");

    description.AddRequiredFile(OptimizeAreaWayIdsGenerator::WAYS_TMP);

    description.AddProvidedFile(WayDataFile::WAYS_DAT);
    description.AddProvidedDebuggingFile(WayDataFile::WAYS_IDMAP);
    description.AddProvidedTemporaryFile(WAYADDRESS_DAT);
  }
}
