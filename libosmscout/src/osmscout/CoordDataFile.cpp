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

#include <osmscout/CoordDataFile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  const char* CoordDataFile::COORD_DAT="coord.dat";

  CoordDataFile::CoordDataFile()
  : isOpen(false),
    pageSize(0)
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
    pageFileOffsetMap.clear();

    try {
      scanner.Open(datafilename,
                   FileScanner::FastRandom,
                   memoryMapedData);

      FileOffset mapOffset;

      scanner.Read(mapOffset);
      scanner.Read(pageSize);

      scanner.SetPos(mapOffset);

      uint32_t mapSize;

      scanner.Read(mapSize);

      for (size_t i=1; i<=mapSize; i++) {
        PageId     pageId;
        FileOffset offset;

        scanner.Read(pageId);
        scanner.Read(offset);

        pageFileOffsetMap[pageId]=offset;
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
    pageFileOffsetMap.clear();

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

  bool CoordDataFile::Get(const std::set<OSMId>& ids, ResultMap& resultMap) const
  {
    assert(isOpen);

    resultMap.clear();
    resultMap.reserve(ids.size());

    try {
      for (const auto& id : ids) {
        PageId relatedId=id+std::numeric_limits<OSMId>::min();
        PageId pageId=relatedId/pageSize;

        auto   pageOffset=pageFileOffsetMap.find(pageId);

        if (pageOffset==pageFileOffsetMap.end()) {
          continue;
        }

        FileOffset offset=pageOffset->second+(relatedId%pageSize)*(coordByteSize+1);

        scanner.SetPos(offset);

        uint8_t  serial;
        bool     isSet;
        GeoCoord coord;

        scanner.Read(serial);
        scanner.ReadConditionalCoord(coord,
                                     isSet);

        if (!isSet) {
          continue;
        }

        resultMap.insert(std::make_pair(id,
                                        Coord(serial,
                                              coord)));
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }

    return true;
  }
}
