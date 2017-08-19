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
    FileWriter                   writer;
    uint32_t                     overallDataCount;
    NameFeatureValueReader       *nameReader;
    PostalCodeFeatureValueReader *postalCodeReader;

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
    postalCodeReader=new PostalCodeFeatureValueReader(typeConfig);

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

      NameFeatureValue       *nameValue=nameReader->GetValue(way.GetFeatureValueBuffer());

      if (nameValue==NULL) {
        return true;
      }

      PostalCodeFeatureValue *postalCodeValue=postalCodeReader->GetValue(way.GetFeatureValueBuffer());
      std::string            name;
      std::string            postalCode;

      name=nameValue->GetName();

      if (postalCodeValue!=NULL) {
        postalCode=postalCodeValue->GetPostalCode();
      }

      writer.WriteFileOffset(offset);
      writer.WriteNumber(way.GetType()->GetWayId());

      writer.Write(name);
      writer.Write(postalCode);

      writer.Write(way.nodes,false);

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

    delete postalCodeReader;
    postalCodeReader=NULL;

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

    if (way.nodes.size()>=2) {
      size_t lastIndex=0;
      size_t currentIndex=1;

      nodeBuffer.clear();

      // Prefill with the first coordinate
      way.nodes[0].GetCoord().EncodeToBuffer(buffers[0]);

      nodeBuffer.push_back(way.nodes[0]);

      for (size_t n=1; n<way.nodes.size(); n++) {
        way.nodes[n].GetCoord().EncodeToBuffer(buffers[currentIndex]);

        if (IsEqual(buffers[lastIndex],
                    buffers[currentIndex])) {
          if (way.GetSerial(n)==0) {
            duplicateCount++;
            reduced=true;
          }
          else if (way.GetSerial(n-1)==0) {
            way.nodes[n-1]=way.nodes[n];
            duplicateCount++;
            reduced=true;
          }
          else {
            nodeBuffer.push_back(way.nodes[n]);

            lastIndex=currentIndex;
            currentIndex=(lastIndex+1)%2;
          }
        }
        else {
          nodeBuffer.push_back(way.nodes[n]);

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
        way.nodes=nodeBuffer;
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
    if (way.nodes.size()<3) {
      return true;
    }

    nodeBuffer.clear();

    size_t last=0;
    size_t current=1;
    bool   reduced=false;

    nodeBuffer.push_back(way.nodes[0]);

    while (current+1<way.nodes.size()) {
      double distance=CalculateDistancePointToLineSegment(way.nodes[current],
                                                          nodeBuffer[last],
                                                          way.nodes[current+1]);

      if (distance<1/latConversionFactor &&
          way.GetSerial(current)==0) {
        reduced=true;
        redundantCount++;
        current++;
      }
      else {
        nodeBuffer.push_back(way.nodes[current]);

        last++;
        current++;
      }
    }

    nodeBuffer.push_back(way.nodes[current]);

    if (reduced) {
      way.nodes=nodeBuffer;
    }

    return true;
  }

  bool WayNodeReductionProcessorFilter::Process(Progress& progress,
                                                const FileOffset& offset,
                                                Way& way,
                                                bool& save)
  {
    overallCount+=way.nodes.size();

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
    coord=data.nodes[0].GetCoord();

    for (size_t n=1; n<data.nodes.size(); n++) {
      coord.Set(std::max(coord.GetLat(),data.nodes[n].GetLat()),
                std::min(coord.GetLon(),data.nodes[n].GetLon()));
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
