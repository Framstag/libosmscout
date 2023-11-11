/*
  This source is part of the libosmscout library
  Copyright (C) 2023 Lukáš Karas

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

#include <osmscoutclient/MapDirectory.h>

#include <osmscoutclient/json/json.hpp>

#include <osmscout/log/Logger.h>
#include <osmscout/io/File.h>
#include <osmscout/util/String.h>

namespace osmscout {

MapDirectory::MapDirectory(const std::filesystem::path &dir):
  dir(dir)
{
  std::vector<std::string> fileNames = MandatoryFiles();
  // text*.dat files are optional, these files are missing when db is build without Marisa support

  osmscout::log.Debug() << "Checking db files in directory " << GetDirStr();
  valid=true;
  for (const auto &fileName: fileNames) {
    bool exists=ExistsInFilesystem((dir / fileName).string());
    if (!exists){
      osmscout::log.Debug() << "Missing mandatory file: " << fileName;
    }
    valid &= exists;
  }
  if (!valid){
    osmscout::log.Warn() << "Can't use db " << GetDirStr() << ", some mandatory files are missing.";
  }

  // metadata
  if (auto metadataPath = dir / FileMetadata;
      ExistsInFilesystem(metadataPath.string())) {
    // we need map metadata even when it is not valid (not fully downloaded for example...)

    std::vector<char> content;
    if (!ReadFile(metadataPath.string(), content)) {
      log.Warn() << "Couldn't open" << metadataPath.string() << "file.";
      return;
    }
    log.Debug() << "Loading map metadata from " << metadataPath.string();

    using json = nlohmann::json;
    try {
      auto metadataObject = json::parse(content);
      if (metadataObject["name"].is_string() &&
          metadataObject["map"].is_string() &&
          metadataObject["creation"].is_number() &&
          metadataObject["version"].is_number()) {

        name = metadataObject["name"].get<std::string>();
        auto list = SplitString(metadataObject["map"].get<std::string>(), "/");
        std::copy(list.begin(), list.end(), std::back_inserter(path));
        using namespace std::chrono;
        creation = time_point<system_clock, seconds>(seconds(metadataObject["creation"].get<uint64_t>()));
        version = metadataObject["version"].get<uint32_t>();
        metadata = true;
      }
    } catch (const json::exception &e) {
      log.Warn() << "Failed to parse json from" << metadataPath.string() << ":" << e.what();
    }
  }
}

std::vector<std::string> MapDirectory::MandatoryFiles()
{
  return std::vector<std::string>{
    "bounding.dat",
    "nodes.dat",
    "areas.dat",
    "ways.dat",
    "areanode.idx",
    "areaarea.idx",
    "areaway.idx",
    "areasopt.dat",
    "waysopt.dat",
    "location.idx",
    "water.idx",
    "intersections.dat",
    "intersections.idx",
    "route.dat",
    "arearoute.idx",
    "router.dat",
    "router2.dat",
    // types.dat should be last, when download is interrupted,
    //directory is not recognized as valid map
    "types.dat"
  };
}

std::vector<std::string> MapDirectory::OptionalFiles(){
  return std::vector<std::string>{{
    "textloc.dat",
    "textother.dat",
    "textpoi.dat",
    "textregion.dat",
    "coverage.idx"
  }};
}

std::vector<std::string> MapDirectory::MetadataFiles()
{
  return std::vector<std::string>{
    FileMetadata
  };
}

uint64_t MapDirectory::ByteSize() const
{
  uint64_t size = 0;
  auto Count = [&size, this](const std::vector<std::string> &fileNames) {
    for (const auto &fileName: fileNames) {
      if(std::filesystem::exists(dir / fileName)){
        size+=std::filesystem::file_size(dir / fileName);
      }
    }
  };

  Count(MandatoryFiles());
  Count(OptionalFiles());
  Count(MetadataFiles());

  return size;
}

bool MapDirectory::DeleteDatabase()
{
  valid=false;

  bool result=true;
  auto DeleteFiles = [&result, this](const std::vector<std::string> &fileNames){
    for (const auto &fileName: fileNames) {
      if(std::filesystem::exists(dir / fileName)) {
        result&=RemoveFile((dir / fileName).string());
      } else {
        // check for partial download
        std::string tempFileName = (dir / fileName).string() + MapDirectory::TemporaryFileSuffix;
        if (std::filesystem::exists(tempFileName)) {
          result &= RemoveFile(tempFileName);
        }
      }
    }
  };

  DeleteFiles(MandatoryFiles());
  DeleteFiles(OptionalFiles());
  DeleteFiles(MetadataFiles());

  result&=RemoveFile(GetDirStr());
  if (result){
    log.Debug() << "Removed db" << GetDirStr();
  }else{
    log.Warn() << "Failed to remove db directory completely" << GetDirStr();
  }
  return result;
}

}
