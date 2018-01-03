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

      data->nodeData.push_back(PreprocessorCallback::RawNodeData(id,coord));

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
      PreprocessorCallback::RawWayData wayData;
      wayData.id=wayId++;

      progress.Info("Generating region '"+region.GetName()+"' "+NumberToString(wayData.id)+"...");

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
        wayData.tags[tagAdminLevel]=NumberToString(region.GetAdminLevel());
      }

      wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopLeft()));
      wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopRight()));
      wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetBottomRight()));
      wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetBottomLeft()));
      wayData.nodes.push_back(RegisterAndGetRawNodeId(data,box.GetTopLeft()));


      data->wayData.push_back(std::move(wayData));

      if (region.GetRegionList().empty()) {
        return;
      }

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
