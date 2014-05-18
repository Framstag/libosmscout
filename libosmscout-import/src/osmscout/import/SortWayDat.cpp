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
    bool Process(const FileOffset& offset,
                 Way& way);
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

  bool WayLocationProcessorFilter::Process(const FileOffset& offset,
                                           Way& way)
  {
    std::string location;

    GetAndEraseTag(way.GetAttributes().GetTags(),
                   tagAddrStreet,
                   location);

    bool isAddress=!location.empty() && !way.GetAddress().empty();
    bool isPoi=!way.GetName().empty() && poiTypes.find(way.GetType())!=poiTypes.end();

    if (!isAddress && !isPoi) {
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

    if (!writer.Write(way.GetAddress())) {
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
  }

  std::string SortWayDataGenerator::GetDescription() const
  {
    return "Sort/copy ways";
  }

}
