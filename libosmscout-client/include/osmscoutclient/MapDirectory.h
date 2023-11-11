#ifndef OSMSCOUT_CLIENT_MAPDIRECTORY_H
#define OSMSCOUT_CLIENT_MAPDIRECTORY_H

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscout/util/Time.h>

#include <vector>
#include <string>
#include <filesystem>

namespace osmscout {

/**
 * Holder for map db metadata
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_API MapDirectory
{
public:
  static constexpr char const *FileMetadata = "metadata.json";
  static constexpr char const *TemporaryFileSuffix = ".download"; ///< suffix of file being downloaded

public:
  MapDirectory() = default;
  explicit MapDirectory(const std::filesystem::path &dir);
  ~MapDirectory() = default;

  MapDirectory(const MapDirectory &other) = default;
  MapDirectory &operator=(const MapDirectory &other) = default;

  MapDirectory(MapDirectory &&other) = default;
  MapDirectory &operator=(MapDirectory &&other) = default;

  std::string GetDirStr() const
  {
    return dir.string();
  }

  std::filesystem::path GetDir() const
  {
    return dir;
  }

  static std::vector<std::string> MandatoryFiles();
  static std::vector<std::string> OptionalFiles();
  static std::vector<std::string> MetadataFiles();

  /**
   * byte size of all db files on disk
   * @return
   */
  uint64_t ByteSize() const;

  /**
   * Delete complete db
   */
  bool DeleteDatabase();

  /**
   * Check if directory contains all required files for osmscout db
   * @return true if all requirements met and directory may be used as db
   */
  bool IsValid() const
  {
    return valid;
  }

  /**
   * Check if map directory contains metadata created by downloader
   * @return true if map directory contains metadata
   */
  bool HasMetadata() const
  {
    return metadata;
  }

  /**
   * Human readable name of the map. It is name of geographical region usually (eg: Germany, Czech Republic...).
   * Name is localised by server when it is downloading. When locale is changed later, name will remain
   * in its original locale.
   * @return map name
   */
  std::string GetName() const
  {
    return name;
  }

  /** Logical path of the map, eg: europe/germany; europe/czech-republic
   */
  std::vector<std::string> GetPath() const
  {
    return path;
  }

  /**
   * Time of map import
   * @return
   */
  Timestamp GetCreation() const
  {
    return creation;
  }

  int GetVersion() const
  {
    return version;
  }

  bool operator<(const MapDirectory &o) const
  {
    if (GetName() == o.GetName()){
      return GetDirStr() < o.GetDirStr();
    }
    return GetName() < o.GetName();
  }

private:
  std::filesystem::path dir;
  bool valid{false};
  bool metadata{false};
  std::string name;
  std::vector<std::string> path;
  Timestamp creation;
  int version{0};
};

}

#endif //OSMSCOUT_CLIENT_MAPDIRECTORY_H
