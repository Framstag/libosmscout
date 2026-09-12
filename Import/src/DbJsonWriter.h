#ifndef OSMSCOUT_IMPORT_DBJSONWRITER_H
#define OSMSCOUT_IMPORT_DBJSONWRITER_H

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

#include <cstdint>
#include <string>
#include <vector>

#include <osmscout/util/GeoBox.h>

namespace osmscout {

  /**
   * One entry of the output file inventory of a generated database.
   */
  struct DbJsonFileEntry
  {
    std::string name;   //!< relative file name
    uint64_t    size=0; //!< file size in bytes
    uint32_t    crc32=0;//!< CRC-32 (zlib-compatible) of the file content
  };

  /**
   * All data written to the db.json metadata file of a generated database.
   */
  struct DbJsonData
  {
    std::string                  generatedAt;       //!< ISO 8601 UTC timestamp
    uint32_t                     typeConfigVersion; //!< FILE_FORMAT_VERSION of the database
    std::string                  sourceUrl;         //!< source download URL, may be empty
    std::string                  sourceMd5;         //!< source integrity hash, may be empty
    std::string                  toolVersion;       //!< import tool version
    size_t                       startStep;        //!< first executed import step
    size_t                       endStep;          //!< last executed import step
    double                       durationSeconds;  //!< import run duration
    GeoBox                       boundingBox;      //!< bounding box of the imported data
    std::vector<DbJsonFileEntry> files;            //!< output file inventory
    size_t                       typeCount;        //!< number of defined types
  };

  /**
   * Builds the output file inventory for the given relative file names
   * inside the destination directory. Files that do not exist are skipped
   * (optional files may be absent).
   *
   * @param destinationDirectory
   *    Directory containing the generated database
   * @param fileNames
   *    Relative file names of the generated data files
   * @param files
   *    On success, the inventory entries for all existing files
   * @return
   *    True on success, else false
   */
  bool BuildDbJsonInventory(const std::string& destinationDirectory,
                            const std::vector<std::string>& fileNames,
                            std::vector<DbJsonFileEntry>& files);

  /**
   * Writes the db.json metadata file into the destination directory.
   * The file is written atomically (temporary file + rename), so a reader
   * never observes a partially written metadata file.
   *
   * @param destinationDirectory
   *    Directory containing the generated database
   * @param data
   *    Metadata content
   * @return
   *    True on success, else false
   */
  bool WriteDbJson(const std::string& destinationDirectory,
                   const DbJsonData& data);
}

#endif //OSMSCOUT_IMPORT_DBJSONWRITER_H
