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

#include <osmscout/import/SortAreaDat.h>

#include <osmscout/GeoCoord.h>

#include <osmscout/TypeFeatures.h>

#include <iostream>
namespace osmscout {

  class AreaLocationProcessorFilter : public SortDataGenerator<Area>::ProcessingFilter
  {
  private:
    FileWriter                 writer;
    uint32_t                   overallDataCount;
    NameFeatureValueReader     *nameReader;
    LocationFeatureValueReader *locationReader;
    AddressFeatureValueReader  *addressReader;

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

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areaaddress.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");

      return false;
    }

    writer.Write(overallDataCount);

    return true;
  }

  bool AreaLocationProcessorFilter::Process(Progress& /*progress*/,
                                            const FileOffset& offset,
                                            Area& area,
                                            bool& /*save*/)
  {
    for (std::vector<Area::Ring>::iterator ring=area.rings.begin();
        ring!=area.rings.end();
        ++ring) {
      NameFeatureValue     *nameValue=nameReader->GetValue(ring->GetFeatureValueBuffer());
      LocationFeatureValue *locationValue=locationReader->GetValue(ring->GetFeatureValueBuffer());
      AddressFeatureValue  *addressValue=addressReader->GetValue(ring->GetFeatureValueBuffer());

      std::string          name;
      std::string          location;
      std::string          address;

      if (nameValue!=NULL) {
        name=nameValue->GetName();
      }

      if (locationValue!=NULL) {
        location=locationValue->GetLocation();
      }

      if (addressValue!=NULL) {
        address=addressValue->GetAddress();
      }

      bool isAddress=!ring->GetType()->GetIgnore() &&
                     !location.empty() &&
                     !address.empty();

      bool isPoi=!name.empty() && ring->GetType()->GetIndexAsPOI();

      size_t locationIndex;

      if (locationReader->GetIndex(ring->GetFeatureValueBuffer(),
                                  locationIndex) &&
          ring->GetFeatureValueBuffer().HasValue(locationIndex)) {
        ring->UnsetFeature(locationIndex);
      }

      if (!isAddress && !isPoi) {
        continue;
      }

      if (ring->ring==Area::masterRingId &&
          ring->nodes.empty()) {
        for (std::vector<Area::Ring>::const_iterator r=area.rings.begin();
            r!=area.rings.end();
            ++r) {
          if (r->ring==Area::outerRingId) {
            if (!writer.WriteFileOffset(offset)) {
              return false;
            }

            if (!writer.WriteNumber(ring->GetType()->GetAreaId())) {
              return false;
            }

            if (!writer.Write(name)) {
              return false;
            }

            if (!writer.Write(location)) {
              return false;
            }

            if (!writer.Write(address)) {
              return false;
            }

            if (!writer.Write(r->nodes)) {
              return false;
            }

            overallDataCount++;
          }
        }
      }
      else {
        if (!writer.WriteFileOffset(offset)) {
          return false;
        }

        if (!writer.WriteNumber(ring->GetType()->GetAreaId())) {
          return false;
        }

        if (!writer.Write(name)) {
          return false;
        }

        if (!writer.Write(location)) {
          return false;
        }

        if (!writer.Write(address)) {
          return false;
        }

        if (!writer.Write(ring->nodes)) {
          return false;
        }

        overallDataCount++;
      }
    }

    return true;
  }

  bool AreaLocationProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                       Progress& /*progress*/,
                                                       const TypeConfig& /*typeConfig*/)
  {
    delete nameReader;
    nameReader=NULL;

    delete locationReader;
    locationReader=NULL;

    delete addressReader;
    addressReader=NULL;

    writer.SetPos(0);
    writer.Write(overallDataCount);

    return writer.Close();
  }

  class AreaNodeReductionProcessorFilter : public SortDataGenerator<Area>::ProcessingFilter
  {
  private:
    std::vector<GeoCoord> nodeBuffer;
    std::vector<Id>       idBuffer;
    size_t                duplicateCount;
    size_t                redundantCount;
    size_t                overallCount;

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
        idBuffer.clear();

        ring->nodes[0].EncodeToBuffer(buffers[0]);

        nodeBuffer.push_back(ring->nodes[0]);
        if (!ring->ids.empty()) {
          idBuffer.push_back(ring->ids[0]);
        }

        for (size_t n=1; n<ring->nodes.size(); n++) {
          ring->nodes[n].EncodeToBuffer(buffers[currentIndex]);

          if (IsEqual(buffers[lastIndex],
                      buffers[currentIndex])) {
            if (n>=ring->ids.size() ||
                ring->ids[n]==0) {
              reduced=true;
            }
            else if ((n-1)>=ring->ids.size() ||
                ring->ids[n-1]==0) {
              ring->ids[n-1]=ring->ids[n];
              reduced=true;
            }
            else {
              nodeBuffer.push_back(ring->nodes[n]);
              if (n<ring->ids.size()) {
                idBuffer.push_back(ring->ids[n]);
              }

              lastIndex=currentIndex;
              currentIndex=(lastIndex+1)%2;
            }
          }
          else {
            nodeBuffer.push_back(ring->nodes[n]);
            if (n<ring->ids.size()) {
              idBuffer.push_back(ring->ids[n]);
            }

            lastIndex=currentIndex;
            currentIndex=(lastIndex+1)%2;
          }
        }
      }

      if (reduced) {
        if (nodeBuffer.size()<3) {
          progress.Debug("Area " + NumberToString(offset) + " empty/invalid ring removed after node reduction");
          ring=area.rings.erase(ring);

          if (area.rings.size()==0 ||
              (area.rings.size()==1 &&
               area.rings[0].ring==Area::masterRingId   )) {
              save=false;
              return true;
          }
        }
        else {
          ring->nodes=nodeBuffer;
          ring->ids=idBuffer;
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
      idBuffer.clear();

      size_t last=0;
      size_t current=1;
      bool   reduced=false;

      nodeBuffer.push_back(ring.nodes[0]);
      if (!ring.ids.empty()) {
        idBuffer.push_back(ring.ids[0]);
      }

      while (current+1<ring.nodes.size()) {
        double distance=CalculateDistancePointToLineSegment(ring.nodes[current],
                                                            nodeBuffer[last],
                                                            ring.nodes[current+1]);

        if (distance<1/latConversionFactor &&
            (current>=ring.ids.size() || ring.ids[current]==0)) {
          reduced=true;
          redundantCount++;
          current++;
        }
        else {
          nodeBuffer.push_back(ring.nodes[current]);
          if (!ring.ids.empty()) {
            idBuffer.push_back(ring.ids[current]);
          }

          last++;
          current++;
        }
      }

      nodeBuffer.push_back(ring.nodes[current]);
      if (!ring.ids.empty()) {
        idBuffer.push_back(ring.ids[current]);
      }

      if (reduced) {
        ring.nodes=nodeBuffer;
        ring.ids=idBuffer;
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
    progress.Info("Duplicate nodes removed: " + NumberToString(duplicateCount));
    progress.Info("Redundant nodes removed: " + NumberToString(redundantCount));
    progress.Info("Overall nodes: " + NumberToString(overallCount));

    return true;
  }

  void SortAreaDataGenerator::GetTopLeftCoordinate(const Area& data,
                                                   double& maxLat,
                                                   double& minLon)
  {
    bool start=true;
    for (size_t r=0; r<data.rings.size(); r++) {
      if (data.rings[r].ring==Area::outerRingId) {
        for (size_t n=0; n<data.rings[r].nodes.size(); n++) {
          if (start) {
            maxLat=data.rings[r].nodes[n].GetLat();
            minLon=data.rings[r].nodes[n].GetLon();

            start=false;
          }
          else {
            maxLat=std::max(maxLat,data.rings[r].nodes[n].GetLat());
            minLon=std::min(minLon,data.rings[r].nodes[n].GetLon());
          }
        }
      }
    }
  }

  SortAreaDataGenerator::SortAreaDataGenerator()
  : SortDataGenerator<Area>("areas.dat","areas.idmap")
  {
    AddSource("areas3.tmp");

    AddFilter(std::make_shared<AreaLocationProcessorFilter>());
    AddFilter(std::make_shared<AreaNodeReductionProcessorFilter>());
  }

  std::string SortAreaDataGenerator::GetDescription() const
  {
    return "Sort/copy areas";
  }
}
