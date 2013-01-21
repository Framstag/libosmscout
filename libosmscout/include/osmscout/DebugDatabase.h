#ifndef OSMSCOUT_DEBUGDATABASE_H
#define OSMSCOUT_DEBUGDATABASE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <list>
#include <set>

// Type and style sheet configuration
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

#include <osmscout/Point.h>

namespace osmscout {

  /**
    Database instance initialisation parameter to influence the behaviour of the database
    instance.

    The following groups attributes are currently available:
    * cache sizes.
    */
  class OSMSCOUT_API DebugDatabaseParameter
  {
  private:

  public:
    DebugDatabaseParameter();

  };

  class OSMSCOUT_API DebugDatabase
  {
  private:
    bool                      isOpen;          //! true, if opened

    std::string               path;             //! Path to the directory containing all files

    TypeConfig                *typeConfig;      //! Type config for the currently opened map

  public:
    DebugDatabase(const DebugDatabaseParameter& parameter);
    virtual ~DebugDatabase();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    TypeConfig* GetTypeConfig() const;

    bool GetCoords(const std::vector<Id>& ids,
                   std::vector<Point>& coords) const;

    bool ResolveNodeIdsAndOffsets(const std::set<Id>& ids,
                                  std::map<Id,FileOffset>& idFileOffsetMap,
                                  const std::set<FileOffset>& fileOffsets,
                                  std::map<FileOffset,Id>& fileOffsetIdMap);

    bool ResolveWayIdsAndOffsets(const std::set<Id>& ids,
                                 std::map<Id,FileOffset>& idFileOffsetMap,
                                 const std::set<FileOffset>& fileOffsets,
                                 std::map<FileOffset,Id>& fileOffsetIdMap);

    bool ResolveRelationIdsAndOffsets(const std::set<Id>& ids,
                                      std::map<Id,FileOffset>& idFileOffsetMap,
                                      const std::set<FileOffset>& fileOffsets,
                                      std::map<FileOffset,Id>& fileOffsetIdMap);
  };
}

#endif
