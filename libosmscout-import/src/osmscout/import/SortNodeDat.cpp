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

#include <osmscout/import/SortNodeDat.h>

namespace osmscout {
  class NodeLocationProcessorFilter : public SortDataGenerator<Node>::ProcessingFilter
  {
  private:
    FileWriter                 writer;
    uint32_t                   overallDataCount;
    OSMSCOUT_HASHSET<TypeId>   poiTypes;
    TagId                      tagAddrStreet;
    NameFeatureValueReader     *nameReader;
    LocationFeatureValueReader *locationReader;
    AddressFeatureValueReader  *addressReader;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig);
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Node& node,
                 bool& save);
    bool AfterProcessingEnd();
  };

  bool NodeLocationProcessorFilter::BeforeProcessingStart(const ImportParameter& parameter,
                                                         Progress& progress,
                                                         const TypeConfig& typeConfig)
  {
    overallDataCount=0;

    typeConfig.GetIndexAsPOITypes(poiTypes);

    nameReader=new NameFeatureValueReader(typeConfig);
    locationReader=new LocationFeatureValueReader(typeConfig);
    addressReader=new AddressFeatureValueReader(typeConfig);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "nodeaddress.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");

      return false;
    }

    writer.Write(overallDataCount);

    return true;
  }

  bool NodeLocationProcessorFilter::Process(Progress& /*progress*/,
                                            const FileOffset& offset,
                                            Node& node,
                                            bool& /*save*/)
  {
    NameFeatureValue     *nameValue=nameReader->GetValue(node.GetFeatureValueBuffer());
    LocationFeatureValue *locationValue=locationReader->GetValue(node.GetFeatureValueBuffer());
    AddressFeatureValue  *addressValue=addressReader->GetValue(node.GetFeatureValueBuffer());

    bool isAddress=addressValue!=NULL;
    bool isPoi=nameValue!=NULL &&
               poiTypes.find(node.GetTypeId())!=poiTypes.end();

    std::string name;
    std::string location;
    std::string address;

    if (nameValue!=NULL) {
      name=nameValue->GetName();
    }

    if (addressValue!=NULL && locationValue!=NULL) {
      location=locationValue->GetLocation();
      address=addressValue->GetAddress();
    }

    if (!isAddress && !isPoi) {
      return true;
    }

    if (!writer.WriteFileOffset(offset)) {
      return false;
    }

    if (!writer.WriteNumber(node.GetTypeId())) {
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

    if (!writer.WriteCoord(node.GetCoords())) {
      return false;
    }

    if (locationValue!=NULL) {
      size_t locationIndex;

      if (locationReader->GetIndex(node.GetFeatureValueBuffer(),
                                   locationIndex)) {
        node.FreeFeatureValue(locationIndex);
      }
    }

    overallDataCount++;

    return true;
  }

  bool NodeLocationProcessorFilter::AfterProcessingEnd()
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

  void SortNodeDataGenerator::GetTopLeftCoordinate(const Node& data,
                                                   double& maxLat,
                                                   double& minLon)
  {
    maxLat=data.GetLat();
    minLon=data.GetLon();
  }

  SortNodeDataGenerator::SortNodeDataGenerator()
  : SortDataGenerator<Node>("nodes.dat","nodes.idmap")
  {
    AddSource(osmRefNode,"nodes.tmp");

    AddFilter(new NodeLocationProcessorFilter());
  }

  std::string SortNodeDataGenerator::GetDescription() const
  {
    return "Sort/copy nodes";
  }
}
