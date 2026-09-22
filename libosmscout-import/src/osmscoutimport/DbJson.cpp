/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscoutimport/DbJson.h>

#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <osmscout/io/Crc32.h>
#include <osmscout/io/File.h>
#include <osmscout/GeoCoord.h>

#include <osmscoutimport/JsonWriter.h>

namespace osmscout {

  /**
   * The only db.json schema version this library knows how to read.
   */
  namespace {
    constexpr uint64_t kSupportedDbJsonSchema=1;
  }

  bool BuildDbJsonInventory(const std::string& destinationDirectory,
                            const std::vector<std::string>& fileNames,
                            std::vector<DbJsonFileEntry>& files)
  {
    files.clear();

    for (const auto& name : fileNames) {
      std::string path=AppendFileToDir(destinationDirectory,
                                       name);

      if (!ExistsInFilesystem(path)) {
        continue;
      }

      DbJsonFileEntry entry;

      entry.name=name;
      entry.size=static_cast<uint64_t>(GetFileSize(path));

      if (!ComputeFileCrc32(path,
                            entry.crc32)) {
        return false;
      }

      files.push_back(entry);
    }

    return true;
  }

  bool WriteDbJson(const std::string& destinationDirectory,
                   const DbJsonData& data)
  {
    std::string tmpPath=AppendFileToDir(destinationDirectory,
                                        "db.json.tmp");
    std::string dbJsonPath=AppendFileToDir(destinationDirectory,
                                           "db.json");

    std::ofstream out(tmpPath,
                      std::ios::out|std::ios::trunc);

    if (!out) {
      return false;
    }

    JsonWriter writer(out);

    writer.BeginObject();

    writer.Key("schema");
    writer.Value(static_cast<uint64_t>(1));

    writer.Key("typeConfigVersion");
    writer.Value(static_cast<uint64_t>(data.typeConfigVersion));

    writer.Key("generatedAt");
    writer.Value(data.generatedAt);

    if (!data.sourceUrl.empty() || !data.sourceMd5.empty()) {
      writer.Key("source");
      writer.BeginObject();

      if (!data.sourceUrl.empty()) {
        writer.Key("url");
        writer.Value(data.sourceUrl);
      }

      if (!data.sourceMd5.empty()) {
        writer.Key("md5");
        writer.Value(data.sourceMd5);
      }

      writer.EndObject();
    }

    writer.Key("import");
    writer.BeginObject();

    writer.Key("tool");
    writer.Value("Import");

    writer.Key("version");
    writer.Value(data.toolVersion);

    writer.Key("startStep");
    writer.Value(static_cast<uint64_t>(data.startStep));

    writer.Key("endStep");
    writer.Value(static_cast<uint64_t>(data.endStep));

    writer.Key("durationSeconds");
    writer.Value(data.durationSeconds);

    writer.EndObject();

    writer.Key("output");
    writer.BeginObject();

    writer.Key("boundingBox");
    writer.BeginObject();

    writer.Key("minLon");
    writer.Value(data.boundingBox.GetMinLon());

    writer.Key("minLat");
    writer.Value(data.boundingBox.GetMinLat());

    writer.Key("maxLon");
    writer.Value(data.boundingBox.GetMaxLon());

    writer.Key("maxLat");
    writer.Value(data.boundingBox.GetMaxLat());

    writer.EndObject();

    writer.Key("files");
    writer.BeginObject();

    for (const auto& file : data.files) {
      writer.Key(file.name);
      writer.BeginObject();

      writer.Key("size");
      writer.Value(file.size);

      writer.Key("crc32");
      writer.Value(static_cast<uint64_t>(file.crc32));

      writer.EndObject();
    }

    writer.EndObject();

    writer.EndObject();

    writer.Key("stats");
    writer.BeginObject();

    writer.Key("types");
    writer.Value(static_cast<uint64_t>(data.typeCount));

    writer.EndObject();

    writer.EndObject();

    out << "\n";
    out.flush();

    if (!out) {
      return false;
    }

    out.close();

    if (!RenameFile(tmpPath,
                    dbJsonPath)) {
      return false;
    }

    return true;
  }

  bool ReadDbJson(const std::string& destinationDirectory,
                  DbJsonData& data)
  {
    using json=nlohmann::json;

    std::string dbJsonPath=AppendFileToDir(destinationDirectory,
                                           "db.json");

    std::ifstream in(dbJsonPath,
                     std::ios::in|std::ios::binary);

    if (!in) {
      return false;
    }

    // The metadata is machine-generated but may come from another version of
    // the tool, so a malformed file is a reported failure, not an exception.
    json document=json::parse(in,
                              nullptr,
                              false);

    in.close();

    if (document.is_discarded() || !document.is_object()) {
      return false;
    }

    auto schema=document.find("schema");

    if (schema==document.end() ||
        !schema->is_number_unsigned() ||
        schema->get<uint64_t>()!=kSupportedDbJsonSchema) {
      return false;
    }

    auto typeConfigVersion=document.find("typeConfigVersion");

    if (typeConfigVersion==document.end() ||
        !typeConfigVersion->is_number_unsigned()) {
      return false;
    }

    auto generatedAt=document.find("generatedAt");

    if (generatedAt==document.end() ||
        !generatedAt->is_string()) {
      return false;
    }

    DbJsonData result;

    result.typeConfigVersion=typeConfigVersion->get<uint32_t>();
    result.generatedAt=generatedAt->get<std::string>();

    auto source=document.find("source");

    if (source!=document.end() && source->is_object()) {
      auto url=source->find("url");
      auto md5=source->find("md5");

      if (url!=source->end() && url->is_string()) {
        result.sourceUrl=url->get<std::string>();
      }

      if (md5!=source->end() && md5->is_string()) {
        result.sourceMd5=md5->get<std::string>();
      }
    }

    auto import=document.find("import");

    if (import!=document.end() && import->is_object()) {
      auto version=import->find("version");
      auto startStep=import->find("startStep");
      auto endStep=import->find("endStep");
      auto durationSeconds=import->find("durationSeconds");

      if (version!=import->end() && version->is_string()) {
        result.toolVersion=version->get<std::string>();
      }

      if (startStep!=import->end() && startStep->is_number_unsigned()) {
        result.startStep=startStep->get<size_t>();
      }

      if (endStep!=import->end() && endStep->is_number_unsigned()) {
        result.endStep=endStep->get<size_t>();
      }

      if (durationSeconds!=import->end() && durationSeconds->is_number()) {
        result.durationSeconds=durationSeconds->get<double>();
      }
    }

    auto output=document.find("output");

    if (output==document.end() || !output->is_object()) {
      return false;
    }

    auto boundingBox=output->find("boundingBox");

    if (boundingBox==output->end() || !boundingBox->is_object()) {
      return false;
    }

    auto minLon=boundingBox->find("minLon");
    auto minLat=boundingBox->find("minLat");
    auto maxLon=boundingBox->find("maxLon");
    auto maxLat=boundingBox->find("maxLat");

    if (minLon==boundingBox->end() || !minLon->is_number() ||
        minLat==boundingBox->end() || !minLat->is_number() ||
        maxLon==boundingBox->end() || !maxLon->is_number() ||
        maxLat==boundingBox->end() || !maxLat->is_number()) {
      return false;
    }

    result.boundingBox.Set(GeoCoord(minLat->get<double>(),
                                    minLon->get<double>()),
                           GeoCoord(maxLat->get<double>(),
                                    maxLon->get<double>()));

    auto files=output->find("files");

    if (files==output->end() || !files->is_object()) {
      return false;
    }

    for (const auto& [name, entry] : files->items()) {
      if (!entry.is_object()) {
        return false;
      }

      auto size=entry.find("size");
      auto crc32=entry.find("crc32");

      if (size==entry.end() || !size->is_number_unsigned() ||
          crc32==entry.end() || !crc32->is_number_unsigned()) {
        return false;
      }

      DbJsonFileEntry fileEntry;

      fileEntry.name=name;
      fileEntry.size=size->get<uint64_t>();
      fileEntry.crc32=crc32->get<uint32_t>();

      result.files.push_back(fileEntry);
    }

    auto stats=document.find("stats");

    if (stats!=document.end() && stats->is_object()) {
      auto types=stats->find("types");

      if (types!=stats->end() && types->is_number_unsigned()) {
        result.typeCount=types->get<size_t>();
      }
    }

    data=result;

    return true;
  }

  bool AddDbJsonInventoryEntry(const std::string& destinationDirectory,
                               const std::string& fileName,
                               bool& updated)
  {
    updated=false;

    std::string dbJsonPath=AppendFileToDir(destinationDirectory,
                                           "db.json");

    // A database built without metadata stays without it: the tool that adds a
    // file is not the one that decides whether the database carries metadata.
    if (!ExistsInFilesystem(dbJsonPath)) {
      return true;
    }

    DbJsonData data;

    if (!ReadDbJson(destinationDirectory,
                    data)) {
      return false;
    }

    std::vector<DbJsonFileEntry> entries;

    if (!BuildDbJsonInventory(destinationDirectory,
                              {fileName},
                              entries)) {
      return false;
    }

    // BuildDbJsonInventory skips missing files, and the file this call is
    // about has to exist by now.
    if (entries.empty()) {
      return false;
    }

    std::erase_if(data.files,
                  [&fileName](const DbJsonFileEntry& entry) {
                    return entry.name==fileName;
                  });

    data.files.push_back(entries.front());

    if (!WriteDbJson(destinationDirectory,
                     data)) {
      return false;
    }

    updated=true;

    return true;
  }
}
