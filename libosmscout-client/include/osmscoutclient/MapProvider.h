#ifndef OSMSCOUT_CLIENT_MAPPROVIDER_H
#define OSMSCOUT_CLIENT_MAPPROVIDER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016 Lukas Karas

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

#include <osmscoutclient/json/json_fwd.hpp>

#include <osmscout/util/String.h>

namespace osmscout {

/**
 * \ingroup ClientAPI
 */
struct OSMSCOUT_CLIENT_API MapProvider
{

private:
  bool valid=false;
  std::string uri;
  std::string listUri;
  std::string name;

public:
  MapProvider() = default;
  MapProvider(const MapProvider &) = default;
  MapProvider(MapProvider &&) = default;

  MapProvider(const std::string &name, const std::string &uri, const std::string &listUri):
    valid(true), uri(uri), listUri(listUri), name(name) {}

  virtual ~MapProvider() = default;

  MapProvider& operator=(const MapProvider &) = default;
  MapProvider& operator=(MapProvider &&) = default;

  std::string getName() const
  {
    return name;
  }

  std::string getUri() const
  {
    return uri;
  }

  std::string getListUri(int fromVersion, int toVersion, std::string locale="en") const
  {
    std::string res = listUri;
    res=ReplaceString(res, "%1", std::to_string(fromVersion));
    res=ReplaceString(res, "%2", std::to_string(toVersion));
    res=ReplaceString(res, "%3", locale);
    return res;
  }

  bool isValid() const
  {
    return valid;
  }

  static MapProvider fromJson(const nlohmann::json &obj);
};

}

#endif // OSMSCOUT_CLIENT_MAPPROVIDER_H
