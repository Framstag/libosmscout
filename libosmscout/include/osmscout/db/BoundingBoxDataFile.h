#ifndef OSMSCOUT_BOUNDINGBOXDATAFILE_H
#define OSMSCOUT_BOUNDINGBOXDATAFILE_H

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

#include <osmscout/lib/CoreImportExport.h>

#include <memory>

#include <osmscout/util/GeoBox.h>

namespace osmscout {

  /**
   * \ingroup Database
   *
   * DataFile class for loading the bounding box of the db.
   * The bounding box is represented by a simple GeoBox.
   */
  class OSMSCOUT_API BoundingBoxDataFile final
  {
  public:
    static const char* const BOUNDINGBOX_DAT;

  private:
    bool        isLoaded;    //!< If true, data has been successfully loaded
    std::string filename;    //!< complete filename for data file
    GeoBox      boundingBox; //!< Bounding box

  public:
    BoundingBoxDataFile();

    bool Load(const std::string& path);

    inline bool IsLoaded() const
    {
      return isLoaded;
    }

    inline std::string GetFilename() const
    {
      return filename;
    }

    inline GeoBox GetBoundingBox() const
    {
      return boundingBox;
    }
  };

  using BoundingBoxDataFileRef = std::shared_ptr<BoundingBoxDataFile>;
}

#endif
