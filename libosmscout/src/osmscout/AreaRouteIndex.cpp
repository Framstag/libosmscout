/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/AreaRouteIndex.h>
#include <osmscout/RouteDataFile.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

namespace osmscout {


  AreaRouteIndex::~AreaRouteIndex()
  {
    Close();
  }

  void AreaRouteIndex::Close()
  {
  }

  bool AreaRouteIndex::Open(const TypeConfigRef& typeConfig,
                            const std::string& path,
                            bool memoryMappedData)
  {
    datafilename=AppendFileToDir(path,RouteDataFile::ROUTE_DAT);
    this->memoryMappedData=memoryMappedData;
    this->typeConfig=typeConfig;
    return true;
  }

  bool AreaRouteIndex::GetOffsets(const GeoBox& boundingBox,
                                  const TypeInfoSet& types,
                                  std::vector<FileOffset>& offsets,
                                  TypeInfoSet& loadedTypes) const
  {
    try {
      // TODO: implement real index
      FileScanner scanner;
      scanner.Open(datafilename,FileScanner::Sequential,memoryMappedData);

      uint32_t entries;

      scanner.Read(entries);

      offsets.reserve(entries);

      for (size_t i=0; i < entries; i++) {
        Route route;
        route.Read(*typeConfig, scanner);
        if (boundingBox.Intersects(route.GetBoundingBox()) &&
            types.IsSet(route.GetType())){
          offsets.push_back(route.GetFileOffset());
          loadedTypes.Set(route.GetType());
        }
      }

      bool err=!scanner.HasError();
      scanner.Close();
      return err;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }
  }

}
