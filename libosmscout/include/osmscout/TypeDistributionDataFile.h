#ifndef OSMSCOUT_TYPEDISTRIBUTIONDATAFILE_H
#define OSMSCOUT_TYPEDISTRIBUTIONDATAFILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscout/CoreImportExport.h>

#include <memory>
#include <vector>

#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * \ingroup Database
   *
   * DataFile class for loading the bounding box of the database.
   * The bounding box is represented by a simple GeoBox.
   */
  class OSMSCOUT_API TypeDistributionDataFile final
  {
  public:
    static const char* const DISTRIBUTION_DAT;

  public:
    struct Distribution
    {
      uint32_t nodeCount;
      uint32_t wayCount;
      uint32_t areaCount;

      Distribution();
    };

  private:
    bool                      isLoaded;    //!< If true, data has been successfully loaded
    std::string               filename;    //!< complete filename for data file

    std::vector<Distribution> distribution; //!< List of type distributions per type

  public:
    TypeDistributionDataFile();

    bool Load(const TypeConfig& typeConfig,
              const std::string& path);

    inline bool IsLoaded() const
    {
      return isLoaded;
    }

    inline std::string GetFilename() const
    {
      return filename;
    }

    inline uint32_t GetNodeCount(const TypeInfo& type) const
    {
      return distribution[type.GetIndex()].nodeCount;
    }

    inline uint32_t GetWayCount(const TypeInfo& type) const
    {
      return distribution[type.GetIndex()].wayCount;
    }

    inline uint32_t GetAreaCount(const TypeInfo& type) const
    {
      return distribution[type.GetIndex()].areaCount;
    }
  };

  using TypeDistributionDataFileRef = std::shared_ptr<TypeDistributionDataFile>;
}

#endif
