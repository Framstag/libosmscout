/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/PreprocessPBF.h>

#include <osmscout/private/Config.h>

#include <cstdio>

// We should try to get rid of this!
#include <arpa/inet.h>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>
#include <osmscout/Util.h>

#include <osmscout/pbf/fileformat.pb.h>

#include <iostream>

namespace osmscout {

  std::string PreprocessPBF::GetDescription() const
  {
    return "PreprocessPBF";
  }

  bool PreprocessPBF::Import(const ImportParameter& parameter,
                             Progress& progress,
                             const TypeConfig& typeConfig)
  {
    progress.SetAction(std::string("Parsing PBF file '")+parameter.GetMapfile()+"'");

    FILE* file;

    file=fopen(parameter.GetMapfile().c_str(),"rb");

    if (file==NULL) {
      progress.Error("Cannot open file!");
      return false;
    }

    char blockHeaderLength[4];

    if (fread(blockHeaderLength,sizeof(char),4,file)!=4) {
      progress.Error("Cannot read block header length!");
      fclose(file);
      return false;
    }

    // ugly!
    uint32_t length=ntohl(*((uint32_t*)&blockHeaderLength));

    PBF::BlockHeader blockHeader;

    char *buffer;

    buffer=new char[length];

    if (fread(buffer,sizeof(char),length,file)!=length) {
      progress.Error("Cannot read block header!");
      delete[] buffer;
      fclose(file);
      return false;
    }

    if (!blockHeader.ParseFromArray(buffer,length)) {
      progress.Error("Cannot parse block header!");
      delete[] buffer;
      fclose(file);
      return false;
    }

    delete[] buffer;

    if (blockHeader.type()!="OSMHeader") {
      progress.Error("File is not an OSM PBF file!");
      fclose(file);
      return false;
    }

    fclose(file);

    /*
    progress.Info(std::string("Nodes:          ")+NumberToString(pp.nodeCount));
    progress.Info(std::string("Ways/Areas/Sum: ")+NumberToString(pp.wayCount)+" "+
                  NumberToString(pp.areaCount)+" "+
                  NumberToString(pp.wayCount+pp.areaCount));
    progress.Info(std::string("Relations:      ")+NumberToString(pp.relationCount));*/

    return false;
  }
}

