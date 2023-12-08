#ifndef OSMSCOUT_CLIENT_LOCATIONDESCRIPTION_H
#define OSMSCOUT_CLIENT_LOCATIONDESCRIPTION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2023  Lukáš Karas

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/AdminRegionInfo.h>

#include <string>

namespace osmscout {

/**
 * \ingroup ClientAPI
 */
struct OSMSCOUT_CLIENT_API LocationInfo {

  enum class Type {
    typeNone,
    typeObject,
    typeCoordinate
  };

  Type                            type;
  std::string                     label;
  std::string                     altName; // name in alternative language
  std::string                     objectType;
  std::vector<AdminRegionInfoRef> adminRegionList;
  std::string                     database;
  std::vector<ObjectFileRef>      references;
  GeoCoord                        coord;
  GeoBox                          bbox;

};

}
#endif //OSMSCOUT_CLIENT_LOCATIONDESCRIPTION_H
