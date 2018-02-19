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

#include <osmscout/TypeFeatures.h>

#include <osmscout/NodeDataFile.h>
#include <osmscout/import/GenNodeDat.h>

namespace osmscout {
  const char* SortNodeDataGenerator::NODEADDRESS_DAT="nodeaddress.dat";

  class NodeLocationProcessorFilter : public SortDataGenerator<Node>::ProcessingFilter
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
                               const TypeConfig& typeConfig) override;
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Node& node,
                 bool& save) override;
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig) override;
  };

  bool NodeLocationProcessorFilter::BeforeProcessingStart(const ImportParameter& parameter,
                                                         Progress& progress,
                                                         const TypeConfig& typeConfig)
  {
    overallDataCount=0;

    nameReader=new NameFeatureValueReader(typeConfig);
    locationReader=new LocationFeatureValueReader(typeConfig);
    addressReader=new AddressFeatureValueReader(typeConfig);
    postalCodeReader=new PostalCodeFeatureValueReader(typeConfig);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  SortNodeDataGenerator::NODEADDRESS_DAT));

      writer.Write(overallDataCount);
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool NodeLocationProcessorFilter::Process(Progress& progress,
                                            const FileOffset& offset,
                                            Node& node,
                                            bool& /*save*/)
  {
    try {
      NameFeatureValue       *nameValue=nameReader->GetValue(node.GetFeatureValueBuffer());
      LocationFeatureValue   *locationValue=locationReader->GetValue(node.GetFeatureValueBuffer());
      AddressFeatureValue    *addressValue=addressReader->GetValue(node.GetFeatureValueBuffer());
      PostalCodeFeatureValue *postalCodeValue=postalCodeReader->GetValue(node.GetFeatureValueBuffer());

      bool isAddress=false;
      bool isPoi=false;

      if (node.GetType()->GetIndexAsAddress()) {
        isAddress=addressValue!=nullptr;
      }

      if (node.GetType()->GetIndexAsPOI()) {
        isPoi=nameValue!=nullptr;
      }

      std::string name;
      std::string location;
      std::string address;
      std::string postalCode;

      if (nameValue!=nullptr) {
        name=nameValue->GetName();
      }

      if (addressValue!=nullptr &&
          locationValue!=nullptr) {
        location=locationValue->GetLocation();
        address=addressValue->GetAddress();
      }

      if (postalCodeValue!=nullptr) {
        postalCode=postalCodeValue->GetPostalCode();
      }

      // We only need location info during import up to this point
      // Thus we delete it now to safe disk space
      if (locationValue!=nullptr) {
        size_t locationIndex;

        if (locationReader->GetIndex(node.GetFeatureValueBuffer(),
                                     locationIndex) &&
          node.GetFeatureValueBuffer().HasFeature(locationIndex)) {
          node.UnsetFeature(locationIndex);
        }
      }

      // Same for postal code
      if (postalCodeValue!=nullptr) {
        size_t postalCodeIndex;

        if (postalCodeReader->GetIndex(node.GetFeatureValueBuffer(),
                                       postalCodeIndex) &&
          node.GetFeatureValueBuffer().HasFeature(postalCodeIndex)) {
          node.UnsetFeature(postalCodeIndex);
        }
      }

      if (!isAddress && !isPoi) {
        return true;
      }

      writer.WriteFileOffset(offset);

      writer.WriteNumber(node.GetType()->GetNodeId());

      writer.Write(name);
      writer.Write(postalCode);
      writer.Write(location);
      writer.Write(address);

      writer.WriteCoord(node.GetCoords());

      overallDataCount++;
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      return false;
    }

    return true;
  }

  bool NodeLocationProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                       Progress& progress,
                                                       const TypeConfig& /*typeConfig*/)
  {
    delete nameReader;
    nameReader=nullptr;

    delete locationReader;
    locationReader=nullptr;

    delete addressReader;
    addressReader=nullptr;

    delete postalCodeReader;
    postalCodeReader=nullptr;

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

  class NodeTypeIgnoreProcessorFilter : public SortDataGenerator<Node>::ProcessingFilter
  {
  private:
    uint32_t    removedNodesCount;
    TypeInfoRef typeInfoIgnore;

  public:
    bool BeforeProcessingStart(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig) override;
    bool Process(Progress& progress,
                 const FileOffset& offset,
                 Node& node,
                 bool& save) override;
    bool AfterProcessingEnd(const ImportParameter& parameter,
                            Progress& progress,
                            const TypeConfig& typeConfig) override;
  };

  bool NodeTypeIgnoreProcessorFilter::BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                                            Progress& /*progress*/,
                                                            const TypeConfig& typeConfig)
  {
    removedNodesCount=0;
    typeInfoIgnore=typeConfig.typeInfoIgnore;

    return true;
  }

  bool NodeTypeIgnoreProcessorFilter::Process(Progress& /*progress*/,
                                              const FileOffset& /*offset*/,
                                              Node& node,
                                              bool& save)
  {
    save=node.GetType()!=nullptr &&
         node.GetType()!=typeInfoIgnore;

    if (!save) {
      removedNodesCount++;
    }

    return true;
  }

  bool NodeTypeIgnoreProcessorFilter::AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                                         Progress& progress,
                                                         const TypeConfig& /*typeConfig*/)
  {
    progress.Info("Nodes without a type removed: " + std::to_string(removedNodesCount));

    return true;
  }

  void SortNodeDataGenerator::GetTopLeftCoordinate(const Node& data,
                                                   GeoCoord& coord)
  {
    coord=data.GetCoords();
  }

  SortNodeDataGenerator::SortNodeDataGenerator()
  : SortDataGenerator<Node>(NodeDataFile::NODES_DAT,NodeDataFile::NODES_IDMAP)
  {
    AddSource(NodeDataGenerator::NODES_TMP);

    AddFilter(std::make_shared<NodeLocationProcessorFilter>());
    AddFilter(std::make_shared<NodeTypeIgnoreProcessorFilter>());
  }

  void SortNodeDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                         ImportModuleDescription& description) const
  {
    description.SetName("SortNodeDataGenerator");
    description.SetDescription("Sort nodes to improve lookup");

    description.AddRequiredFile(NodeDataGenerator::NODES_TMP);

    description.AddProvidedFile(NodeDataFile::NODES_DAT);
    description.AddProvidedDebuggingFile(NodeDataFile::NODES_IDMAP);
    description.AddProvidedTemporaryFile(NODEADDRESS_DAT);
  }
}
