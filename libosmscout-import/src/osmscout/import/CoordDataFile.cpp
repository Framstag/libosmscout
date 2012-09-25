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

#include "osmscout/import/CoordDataFile.h"

#include <cassert>

#include <osmscout/util/File.h>

namespace osmscout {

  CoordDataFile::CoordDataFile(const std::string& datafile)
  : isOpen(false),
    datafile(datafile)
  {
    // no code
  }


  CoordDataFile::~CoordDataFile()
  {
    if (isOpen) {
      Close();
    }
  }


  bool CoordDataFile::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,datafile);

    isOpen=false;
    coordPageOffsetMap.clear();

    if (scanner.Open(datafilename,FileScanner::FastRandom,false)) {
      FileOffset mapOffset;

      if (!scanner.Read(coordPageSize)) {
        Close();

        return false;
      }

      if (!scanner.Read(mapOffset)) {
        Close();

        return false;
      }

      if (!scanner.SetPos(mapOffset)) {
        Close();

        return false;
      }

      uint32_t mapSize;

      if (!scanner.Read(mapSize)) {
        Close();

        return false;
      }

      for (size_t i=1; i<=mapSize; i++) {
        Id         id;
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

    return isOpen;
  }

  bool CoordDataFile::Close()
  {
    bool success=true;

    coordPageOffsetMap.clear();

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    isOpen=false;

    return success;
  }
}

