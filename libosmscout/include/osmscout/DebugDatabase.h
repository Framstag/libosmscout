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

#include <osmscout/ObjectRef.h>

// Type and style sheet configuration
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

#include <osmscout/CoordDataFile.h>

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

  private:
    bool ResolveReferences(const std::string& mapName,
                           RefType fileType,
                           const std::set<ObjectOSMRef>& ids,
                           const std::set<ObjectFileRef>& fileOffsets,
                           std::map<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                           std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap);

  public:
    DebugDatabase(const DebugDatabaseParameter& parameter);
    virtual ~DebugDatabase();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    TypeConfig* GetTypeConfig() const;

    bool GetCoords(std::set<OSMId>& ids,
                   CoordDataFile::CoordResultMap& coordsMap) const;

    bool ResolveReferences(const std::set<ObjectOSMRef>& ids,
                           const std::set<ObjectFileRef>& fileOffsets,
                           std::map<ObjectOSMRef,ObjectFileRef>& idFileOffsetMap,
                           std::map<ObjectFileRef,ObjectOSMRef>& fileOffsetIdMap);
  };
}

#endif
