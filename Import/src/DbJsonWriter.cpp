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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "DbJsonWriter.h"

#include <fstream>
#include <string>
#include <vector>

#include <osmscout/io/Crc32.h>
#include <osmscout/io/File.h>

#include "JsonWriter.h"

namespace osmscout {

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
}
