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

#include <set>
#include <list>
#include <vector>

#include <osmscout/Point.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API CoordDataFile
  {
  private:
    typedef OSMSCOUT_HASHMAP<PageId,FileOffset> CoordPageOffsetMap;

  public:
    struct CoordEntry
    {
      Point point;

      CoordEntry(Id id,
                 double lat,
                 double lon)
      : point(id,lat,lon)
      {
        // no code
      }
    };

    typedef OSMSCOUT_HASHMAP<OSMId,CoordEntry> CoordResultMap;

  private:
    bool                isOpen;             //! If true,the data file is opened
    std::string         datafile;           //! Basename part of the data file name
    std::string         datafilename;       //! complete filename for data file
    mutable FileScanner scanner;            //! File stream to the data file
    uint32_t            coordPageSize;
    CoordPageOffsetMap  coordPageOffsetMap;

  public:
    CoordDataFile(const std::string& datafile);

    virtual ~CoordDataFile();

    bool Open(const std::string& path,
              bool memoryMapedData);
    bool Close();

    std::string GetFilename() const;

    bool Get(std::set<OSMId>& ids,
             CoordResultMap& coordsMap) const;
  };
}

#endif
