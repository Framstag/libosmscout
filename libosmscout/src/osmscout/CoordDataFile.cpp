/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include "osmscout/CoordDataFile.h"

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  const char* CoordDataFile::COORD_DAT="coord.dat";

  CoordDataFile::CoordDataFile()
  : isOpen(false),
    coordPageSize(0)
  {
    // no code
  }

  CoordDataFile::~CoordDataFile()
  {
    if (isOpen) {
      Close();
    }
  }

  bool CoordDataFile::Open(const std::string& path,
                           bool memoryMapedData)
  {
    datafilename=AppendFileToDir(path,COORD_DAT);

    isOpen=false;
    coordPageOffsetMap.clear();

    try {
      scanner.Open(datafilename,
                     FileScanner::FastRandom,
                     memoryMapedData);

      FileOffset mapOffset;

      if (!scanner.Read(coordPageSize)) {
        Close();

        return false;
      }

      if (!scanner.Read(mapOffset)) {
        Close();

        return false;
      }

      scanner.SetPos(mapOffset);

      uint32_t mapSize;

      if (!scanner.Read(mapSize)) {
        Close();

        return false;
      }

      for (size_t i=1; i<=mapSize; i++) {
        PageId     id;
        FileOffset offset;

        if (!scanner.Read(id) ||
            !scanner.Read(offset)) {
          Close();

          return false;
        }

        coordPageOffsetMap[id]=offset;
      }

      isOpen=true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  bool CoordDataFile::Close()
  {
    coordPageOffsetMap.clear();

    try {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      isOpen=false;
      return false;
    }

    return true;
  }

  std::string CoordDataFile::GetFilename() const
  {
    return datafilename;
  }

  bool CoordDataFile::Get(std::set<OSMId>& ids,
                          CoordResultMap& coordsMap) const
  {
    assert(isOpen);

    coordsMap.clear();
    coordsMap.reserve(ids.size());

    for (const auto& id : ids) {
      PageId relatedId=id-std::numeric_limits<Id>::min();
      PageId pageId=relatedId/coordPageSize;

      CoordPageOffsetMap::const_iterator pageOffset=coordPageOffsetMap.find(pageId);

      if (pageOffset!=coordPageOffsetMap.end()) {
        FileOffset offset=pageOffset->second+(relatedId%coordPageSize)*coordByteSize;
        // Number of entry in file (file starts with an empty page we skip)
        PageId     substituteId=(offset-coordPageSize*coordByteSize)/coordByteSize;

        scanner.SetPos(offset);

        bool     isSet;
        GeoCoord coord;

        if (!scanner.ReadConditionalCoord(coord,
                                          isSet)) {
          log.Error() << "Error while reading data from offset " << pageOffset->second << " of file '" << scanner.GetFilename() << "'!";
          scanner.Close();
          return false;
        }

        if (!isSet) {
          continue;
        }

        coordsMap.insert(std::make_pair(id,
                                        CoordEntry(substituteId,
                                                   coord.GetLat(),
                                                   coord.GetLon())));
      }
    }

    return true;
  }
}
