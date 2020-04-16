#ifndef OSMSCOUT_OBJECTVARIANTDATAFILE_H
#define OSMSCOUT_OBJECTVARIANTDATAFILE_H

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

#include <osmscout/routing/RouteNode.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * \ingroup Database
   *
   * DataFile class for loading the object variant data, which is part of the
   * routing graph. The object variant data contains a lookup table for path
   * attributes. Since the number of attribute value combinations is much
   * smaller then the actual number of elements in the route graph it makes
   * sense to store all possible combinations in a lookup table and just
   * index them from the routing graph paths.
   */
  class OSMSCOUT_API ObjectVariantDataFile final
  {
  private:
    std::vector<ObjectVariantData> data; //!< Data loaded

  private:
    bool        isLoaded;                //!< If true, data has been successfully loaded
    std::string filename;                //!< complete filename for data file

  public:
    ObjectVariantDataFile();

    bool Load(const TypeConfig& typeConfig,
              const std::string& filename);

    inline bool IsLoaded() const
    {
      return isLoaded;
    }

    inline std::string GetFilename() const
    {
      return filename;
    }

    inline const std::vector<ObjectVariantData>& GetData() const
    {
      return data;
    }
  };
}

#endif
