#ifndef OSMSCOUT_BASEMAPDATABASE_H
#define OSMSCOUT_BASEMAPDATABASE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

// Type and style sheet configuration
#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>

// Water index
#include <osmscout/WaterIndex.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
    BasemapDatabase instance initialization parameter to influence the behavior of the database
    instance.
    */
  class OSMSCOUT_API BasemapDatabaseParameter CLASS_FINAL
  {
  private:

  public:
    BasemapDatabaseParameter();
  };

  /**
   * \ingroup Database
   *
   * Central access class to all the individual data files and indexes of the basemap.
   *
   * The basemap is a special database holding world-wide information.
   *
   * A database is mainly initialized with a number of optional but performance
   * relevant parameters.
   *
   * The Database is opened by passing the directory that contains
   * all database files.
   */
  class OSMSCOUT_API BasemapDatabase CLASS_FINAL
  {
  private:
    BasemapDatabaseParameter        parameter;                //!< Parameterization of this database object

    std::string                     path;                     //!< Path to the directory containing all files
    bool                            isOpen;                   //!< true, if opened

    mutable WaterIndexRef           waterIndex;               //!< Index of land/sea tiles
    mutable std::mutex              waterIndexMutex;          //!< Mutex to make lazy initialisation of water index thread-safe

  public:
    BasemapDatabase(const BasemapDatabaseParameter& parameter);
    virtual ~BasemapDatabase();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    std::string GetPath() const;

    WaterIndexRef GetWaterIndex() const;
  };

  //! Reference counted reference to an Database instance
  typedef std::shared_ptr<BasemapDatabase> BasemapDatabaseRef;
}

#endif
