#ifndef OSMSCOUT_COORDDATAFILE_H
#define OSMSCOUT_COORDDATAFILE_H

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

#include <osmscout/DataFile.h>
#include <osmscout/Coord.h>

#include <osmscout/util/Cache.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API CoordDataFile : public IndexedDataFile<OSMId,Coord>
  {
  public:
    static const char* COORD_DAT;
    static const char* COORD_IDX;

  public:
    typedef std::unordered_map<OSMId,CoordRef> ResultMap;

  public:
    CoordDataFile(unsigned long indexCacheSize);
  };
}

#endif
