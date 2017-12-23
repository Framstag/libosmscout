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

    void PreprocessOLT::GenerateRegion(const TypeConfigRef& typeConfig,
                                       const ImportParameter& parameter,
                                       Progress& progress,
                                       PreprocessorCallback::RawBlockDataRef data,
                                       const PreprocessorCallback::RawNodeData& topLeft,
                                       const PreprocessorCallback::RawNodeData& topRight,
                                       const PreprocessorCallback::RawNodeData& bottomLeft,
                                       const PreprocessorCallback::RawNodeData& bottomRight,
                                       const Region& region)
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

      wayData.nodes.push_back(topLeft.id);
      wayData.nodes.push_back(topRight.id);
      wayData.nodes.push_back(bottomRight.id);
      wayData.nodes.push_back(bottomLeft.id);
      wayData.nodes.push_back(topLeft.id);

      data->wayData.push_back(std::move(wayData));

      if (region.GetRegionList().empty()) {
        return;
      }

      double latDec=(topLeft.coord.GetLat()-bottomLeft.coord.GetLat())/region.GetRegionList().size();
      size_t currentPos=1;
      PreprocessorCallback::RawNodeData tl=topLeft;
      PreprocessorCallback::RawNodeData tr=topRight;

      for (const auto& childRegion : region.GetRegionList()) {
        PreprocessorCallback::RawNodeData bl;
        PreprocessorCallback::RawNodeData br;

        if (currentPos<region.GetRegionList().size()) {
          bl.id=nodeId++;
          bl.coord.Set(tl.coord.GetLat()-latDec,
                       tl.coord.GetLon());
          data->nodeData.push_back(bl);

          br.id=nodeId++;
          br.coord.Set(tr.coord.GetLat()-latDec,
                       tr.coord.GetLon());
          data->nodeData.push_back(br);
        }
        else {
          bl=bottomLeft;
          br=bottomRight;
        };

        GenerateRegion(typeConfig,
                       parameter,
                       progress,
                       data,
                       tl,
                       tr,
                       bl,
                       br,
                       *childRegion);

        tl=bl;
        tr=br;
        currentPos++;
      }

    }

    void PreprocessOLT::GenerateWaysAndNodes(const TypeConfigRef& typeConfig,
                                             const ImportParameter& parameter,
                                             Progress& progress,
                                             const RegionList& regions)
    {
      progress.SetAction("Generating ways and nodes");

      PreprocessorCallback::RawNodeData topLeft;
      PreprocessorCallback::RawNodeData topRight;

      PreprocessorCallback::RawBlockDataRef data=std::make_shared<PreprocessorCallback::RawBlockData>();

      if (regions.GetRegionList().empty()) {
        progress.Warning("No regions defined, existing...");

        callback.ProcessBlock(data);

        return;
      }

      topLeft.id=nodeId++;
      topLeft.coord.Set(51.0,10.0);
      data->nodeData.push_back(topLeft);

      topRight.id=nodeId++;
      topRight.coord.Set(51.0,11.0);
      data->nodeData.push_back(topRight);

      // We are creating the areas from 50 lat to 51 lat...
      double latDec=1.0/regions.GetRegionList().size();

      for (const auto& region : regions.GetRegionList()) {
        PreprocessorCallback::RawNodeData bottomLeft;
        PreprocessorCallback::RawNodeData bottomRight;

        bottomLeft.id=nodeId++;
        bottomLeft.coord.Set(topLeft.coord.GetLat()-latDec,
                             topLeft.coord.GetLon());
        data->nodeData.push_back(bottomLeft);

        bottomRight.id=nodeId++;
        bottomRight.coord.Set(topRight.coord.GetLat()-latDec,
                              topRight.coord.GetLon());
        data->nodeData.push_back(bottomRight);

        GenerateRegion(typeConfig,
                       parameter,
                       progress,
                       data,
                       topLeft,
                       topRight,
                       bottomLeft,
                       bottomRight,
                       *region);

        topLeft=bottomLeft;
        topRight=bottomRight;
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
