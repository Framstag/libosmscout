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

namespace osmscout {

  class WayLocationProcessorFilter : public SortDataGenerator<Way>::ProcessingFilter
  {
  private:
    FileWriter               writer;
    uint32_t                 overallDataCount;
    OSMSCOUT_HASHSET<TypeId> poiTypes;
    TagId                    tagAddrStreet;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Way& way,
                 bool& save);
    bool AfterProcessingEnd();
  };

  bool WayLocationProcessorFilter::BeforeProcessingStart(const ImportParameter& parameter,
                                                         Progress& progress,
                                                         const TypeConfig& typeConfig)
  {
    overallDataCount=0;

    typeConfig.GetIndexAsPOITypes(poiTypes);
    tagAddrStreet=typeConfig.tagAddrStreet;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayaddress.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");

      return false;
    }

    writer.Write(overallDataCount);

    return true;
  }

  bool WayLocationProcessorFilter::Process(Progress& /*progress*/,
                                           const FileOffset& offset,
                                           Way& way,
                                           bool& /*save*/)
  {
    std::string location;

    GetAndEraseTag(way.GetAttributes().GetTags(),
                   tagAddrStreet,
                   location);

    bool isPoi=!way.GetName().empty() && poiTypes.find(way.GetType())!=poiTypes.end();

    if (!isPoi) {
      return true;
    }

    if (!writer.WriteFileOffset(offset)) {
      return false;
    }

    if (!writer.WriteNumber(way.GetType())) {
      return false;
    }

    if (!writer.Write(way.GetName())) {
      return false;
    }

    if (!writer.Write(location)) {
      return false;
    }

    if (!writer.Write(way.nodes)) {
      return false;
    }

    overallDataCount++;

    return true;
  }

  bool WayLocationProcessorFilter::AfterProcessingEnd()
  {
    writer.SetPos(0);
    writer.Write(overallDataCount);

    return writer.Close();
  }

  class WayNodeReductionProcessorFilter : public SortDataGenerator<Way>::ProcessingFilter
  {
  private:
    std::vector<GeoCoord> nodeBuffer;
    std::vector<Id>       idBuffer;

  private:
    bool IsEqual(const unsigned char buffer1[],
                 const unsigned char buffer2[]);

  public:
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Way& way,
                 bool& save);
  };

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

  bool WayNodeReductionProcessorFilter::Process(Progress& progress,
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
      idBuffer.clear();

      way.nodes[0].EncodeToBuffer(buffers[0]);

      nodeBuffer.push_back(way.nodes[0]);
      if (!way.ids.empty()) {
        idBuffer.push_back(way.ids[0]);
      }

      for (size_t n=1; n<way.nodes.size(); n++) {
        way.nodes[n].EncodeToBuffer(buffers[currentIndex]);

        if (IsEqual(buffers[lastIndex],
                    buffers[currentIndex])) {
          if (n>=way.ids.size() ||
              way.ids[n]==0) {
            reduced=true;
          }
          else if ((n-1)>=way.ids.size() ||
              way.ids[n-1]==0) {
            way.ids[n-1]=way.ids[n];
            reduced=true;
          }
          else {
            nodeBuffer.push_back(way.nodes[n]);
            if (n<way.ids.size()) {
              idBuffer.push_back(way.ids[n]);
            }

            lastIndex=currentIndex;
            currentIndex=(lastIndex+1)%2;
          }
        }
        else {
          nodeBuffer.push_back(way.nodes[n]);
          if (n<way.ids.size()) {
            idBuffer.push_back(way.ids[n]);
          }

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
        way.ids=idBuffer;
      }
    }

    return true;
  }

  void SortWayDataGenerator::GetTopLeftCoordinate(const Way& data,
                                                  double& maxLat,
                                                  double& minLon)
  {
    maxLat=data.nodes[0].GetLat();
    minLon=data.nodes[0].GetLon();

    for (size_t n=1; n<data.nodes.size(); n++) {
      maxLat=std::max(maxLat,data.nodes[n].GetLat());
      minLon=std::min(minLon,data.nodes[n].GetLon());
    }
  }

  SortWayDataGenerator::SortWayDataGenerator()
  : SortDataGenerator<Way>("ways.dat","ways.idmap")
  {
    AddSource(osmRefWay,"wayway.dat");

    AddFilter(new WayLocationProcessorFilter());
    AddFilter(new WayNodeReductionProcessorFilter());
  }

  std::string SortWayDataGenerator::GetDescription() const
  {
    return "Sort/copy ways";
  }

}
