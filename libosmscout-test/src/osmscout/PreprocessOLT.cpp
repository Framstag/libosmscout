/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/PreprocessOLT.h>

#include <osmscout/olt/Scanner.h>
#include <osmscout/olt/Parser.h>

#include <osmscout/util/File.h>

namespace osmscout {
  namespace test {

    PreprocessOLT::PreprocessOLT(PreprocessorCallback& callback)
      : callback(callback)
    {
      // no code
    }

    OSMId PreprocessOLT::RegisterAndGetRawNodeId(PreprocessorCallback::RawBlockDataRef data,
                                                 const GeoCoord& coord)
    {
      auto entry=coordNodeIdMap.find(coord);

      if (entry!=coordNodeIdMap.end()) {
        return entry->second;
      }

      OSMId id=nodeId;

      data->nodeData.emplace_back(id,coord);

      coordNodeIdMap[coord]=id;
      nodeIdCoordMap[id]=coord;

      nodeId++;

      return id;
    }

    void PreprocessOLT::GenerateRegion(const TypeConfigRef& typeConfig,
                                       const ImportParameter& parameter,
                                       Progress& progress,
                                       PreprocessorCallback::RawBlockDataRef data,
                                       const Region& region,
                                       const GeoBox& box,
                                       GeoBoxPartitioner::Direction direction)
    {
      // Generate region as node
      if (region.IsIsNode()) {
        PreprocessorCallback::RawNodeData nodeData;

        nodeData.id=nodeId++;
        nodeData.coord=box.GetCenter();

        progress.Info("Generating node region '"+region.GetName()+"' "+std::to_string(nodeData.id)+"...");

        switch (region.GetPlaceType()) {
        case PlaceType::county:
          nodeData.tags[tagPlace]="county";
          break;
        case PlaceType::region:
          nodeData.tags[tagPlace]="region";
          break;
        case PlaceType::city:
          nodeData.tags[tagPlace]="city";
          break;
        case PlaceType::suburb:
          nodeData.tags[tagPlace]="suburb";
          break;
        case PlaceType::unknown:
          break;
        }

        if (!region.GetName().empty()) {
          nodeData.tags[tagName]=region.GetName();
        }

        if (region.IsBoundary()) {
          progress.Error("region '"+region.GetName()+"' is node and administrative boundary, but only areas can be an boundary");
        }

        data->nodeData.push_back(std::move(nodeData));
      }
        // Generate region as area
      else {
        PreprocessorCallback::RawWayData wayData;

        wayData.id=wayId++;

        progress.Info("Generating area region '"+region.GetName()+"' "+std::to_string(wayData.id)+"...");

        switch (region.GetPlaceType()) {
        case PlaceType::county:
          wayData.tags[tagPlace]="county";
          break;
        case PlaceType::region:
          wayData.tags[tagPlace]="region";
          break;
        case PlaceType::city:
          wayData.tags[tagPlace]="city";
          break;
        case PlaceType::suburb:
          wayData.tags[tagPlace]="suburb";
          break;
        case PlaceType::unknown:
          break;
        }

        if (!region.GetName().empty()) {
          wayData.tags[tagName]=region.GetName();
        }

        if (region.IsBoundary()) {
          wayData.tags[tagBoundary]="administrative";
          wayData.tags[tagAdminLevel]=std::to_string(region.GetAdminLevel());
        }

        wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopLeft()));
        wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopRight()));
        wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetBottomRight()));
        wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetBottomLeft()));
        wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopLeft()));

        data->wayData.push_back(std::move(wayData));
      }

      // Count number of locations in regions

      size_t locationInRegionCount=0;

      for (const auto& postalArea : region.GetPostalAreas()) {
        locationInRegionCount+=postalArea->GetLocations().size();
      }

      if (!region.GetRegionList().empty() &&
          locationInRegionCount>0) {
        progress.Error("Region "+region.GetName()+": Locations are only allowed for leaves of the region tree");
      }

      // Generate a way for each location
      size_t currentLocation=1;
      for (const auto& postalArea : region.GetPostalAreas()) {
        for (const auto& location : postalArea->GetLocations()) {
          PreprocessorCallback::RawWayData locationData;

          locationData.id=wayId++;

          progress.Info("Generating way '"+location->GetName()+"' "+std::to_string(locationData.id)+"...");

          GeoCoord wayLeftCoord(box.GetMinLat()+currentLocation*box.GetHeight()/(locationInRegionCount+1),
                                box.GetMinLon()+box.GetWidth()/10.0);
          GeoCoord wayRightCoord(wayLeftCoord.GetLat(),
                                 box.GetMaxLon()-box.GetWidth()/10.0);

          locationData.nodes.push_back(RegisterAndGetRawNodeId(data,wayLeftCoord));
          locationData.nodes.push_back(RegisterAndGetRawNodeId(data,wayRightCoord));

          locationData.tags[tagHighway]="residential";
          locationData.tags[tagName]=location->GetName();

          if (!postalArea->GetName().empty()) {
            locationData.tags[tagPostalCode]=postalArea->GetName();
          }

          data->wayData.push_back(std::move(locationData));

          size_t currentAddress=1;
          for (const auto& address : location->GetAddresses()) {
            PreprocessorCallback::RawWayData buildingData;

            buildingData.id=wayId++;

            progress.Info("Generating building '"+address->GetName()+"' "+std::to_string(buildingData.id)+"...");

            buildingData.tags[tagBuilding]="yes";
            buildingData.tags[tagAddrCity]=region.GetName();

            if (!postalArea->GetName().empty()) {
              buildingData.tags[tagAddrPostcode]=postalArea->GetName();
            }

            buildingData.tags[tagAddrStreet]=location->GetName();
            buildingData.tags[tagAddrHousenumber]=address->GetName();

            for (const auto& tag : address->GetTags()) {
              buildingData.tags[typeConfig->GetTagId(tag.GetKey())]=tag.GetValue();
            }

            GeoBox buildingBox(GeoCoord(wayLeftCoord.GetLat()+0.0001,
                                        wayLeftCoord.GetLon()+(currentAddress-1)*(wayRightCoord.GetLon()-wayLeftCoord.GetLon())/location->GetAddresses().size()),
                               GeoCoord(wayLeftCoord.GetLat()+0.0002,
                                        wayLeftCoord.GetLon()+currentAddress*(wayRightCoord.GetLon()-wayLeftCoord.GetLon())/location->GetAddresses().size()/2.0));

            buildingData.nodes.push_back(RegisterAndGetRawNodeId(data,buildingBox.GetTopLeft()));
            buildingData.nodes.push_back(RegisterAndGetRawNodeId(data,buildingBox.GetTopRight()));
            buildingData.nodes.push_back(RegisterAndGetRawNodeId(data,buildingBox.GetBottomRight()));
            buildingData.nodes.push_back(RegisterAndGetRawNodeId(data,buildingBox.GetBottomLeft()));
            buildingData.nodes.push_back(RegisterAndGetRawNodeId(data,buildingBox.GetTopLeft()));

            data->wayData.push_back(std::move(buildingData));

            currentAddress++;
          }

          currentLocation++;
        }
      }

      if (region.GetRegionList().empty()) {
        return;
      }

      // Generate sub region

      GeoBoxPartitioner partitioner(box,
                                    direction,
                                    region.GetRegionList().size());

      for (const auto& childRegion : region.GetRegionList()) {
        GeoBox subbox=partitioner.GetCurrentGeoBox();

        GenerateRegion(typeConfig,
                       parameter,
                       progress,
                       data,
                       *childRegion,
                       subbox,
                       direction==GeoBoxPartitioner::Direction::VERTICAL ? GeoBoxPartitioner::Direction::HORIZONTAL : GeoBoxPartitioner::Direction::VERTICAL);

        partitioner.Advance();
      }
    }

    void PreprocessOLT::GenerateWaysAndNodes(const TypeConfigRef& typeConfig,
                                             const ImportParameter& parameter,
                                             Progress& progress,
                                             const RegionList& regions)
    {
      progress.SetAction("Generating ways and nodes");

      GeoBoxPartitioner::Direction direction=GeoBoxPartitioner::Direction::VERTICAL;
      PreprocessorCallback::RawBlockDataRef data=std::make_shared<PreprocessorCallback::RawBlockData>();

      if (regions.GetRegionList().empty()) {
        progress.Warning("No regions defined, existing...");

        callback.ProcessBlock(data);

        return;
      }

      GeoBox box(GeoCoord(50.0,10.0),
                 GeoCoord(51.0,11.0));

      GeoBoxPartitioner partitioner(box,
                                    direction,
                                    regions.GetRegionList().size());

      for (const auto& region : regions.GetRegionList()) {
        GeoBox subbox=partitioner.GetCurrentGeoBox();

        GenerateRegion(typeConfig,
                       parameter,
                       progress,
                       data,
                       *region,
                       subbox,
                       direction==GeoBoxPartitioner::Direction::VERTICAL ? GeoBoxPartitioner::Direction::HORIZONTAL : GeoBoxPartitioner::Direction::VERTICAL);

        partitioner.Advance();
      }

      callback.ProcessBlock(data);
    }

    bool PreprocessOLT::Import(const TypeConfigRef& typeConfig,
                               const ImportParameter& parameter,
                               Progress& progress,
                               const std::string& filename)
    {
      progress.SetAction(std::string("Parsing *.olt file '")+filename+"'");

      tagAdminLevel=typeConfig->GetTagId("admin_level");
      tagBoundary=typeConfig->GetTagId("boundary");
      tagName=typeConfig->GetTagId("name");
      tagPlace=typeConfig->GetTagId("place");
      tagHighway=typeConfig->GetTagId("highway");
      tagPostalCode=typeConfig->GetTagId("postal_code");
      tagBuilding=typeConfig->GetTagId("building");
      tagAddrCity=typeConfig->GetTagId("addr:city");
      tagAddrPostcode=typeConfig->GetTagId("addr:postcode");
      tagAddrStreet=typeConfig->GetTagId("addr:street");
      tagAddrHousenumber=typeConfig->GetTagId("addr:housenumber");

      nodeId=1;
      wayId=1;

      bool success=false;

      try {
        FileOffset fileSize=GetFileSize(filename);
        FILE       * file  =fopen(filename.c_str(),"rb");

        if (file==nullptr) {
          log.Error() << "Cannot open file '" << filename << "'";
          return false;
        }

        auto* content=new unsigned char[fileSize];

        if (fread(content,1,fileSize,file)!=(size_t) fileSize) {
          log.Error() << "Cannot load file '" << filename << "'";
          delete[] content;
          fclose(file);
          return false;
        }

        fclose(file);

        auto* scanner=new olt::Scanner(content,
                                       fileSize);
        auto* parser =new olt::Parser(scanner);

        delete[] content;

        parser->Parse();

        success=!parser->errors->hasErrors;

        if (success) {
          GenerateWaysAndNodes(typeConfig,
                               parameter,
                               progress,
                               parser->GetRegionList());
        }

        delete parser;
        delete scanner;
      }
      catch (IOException& e) {
        log.Error() << e.GetDescription();
      }

      return success;
    }
  }
}
