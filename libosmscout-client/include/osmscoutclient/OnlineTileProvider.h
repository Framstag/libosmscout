#ifndef OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H
#define	OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H

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

#include <string>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Online tile provider object. See OnlineTileProviderModel and Settings.
 */
struct OSMSCOUT_CLIENT_API OnlineTileProvider
{
public:
  OnlineTileProvider() = default;

  OnlineTileProvider(const OnlineTileProvider &) = default;
  OnlineTileProvider(OnlineTileProvider &&) = default;

  OnlineTileProvider(const std::string &id,
                     const std::string &name,
                     const std::vector<std::string> &servers,
                     int maximumZoomLevel,
                     const std::string &copyright):
    valid(true), id(id), name(name), servers(servers), maximumZoomLevel(maximumZoomLevel),
    copyright(copyright){};

  virtual ~OnlineTileProvider() = default;

  OnlineTileProvider& operator=(const OnlineTileProvider &) = default;
  OnlineTileProvider& operator=(OnlineTileProvider &&) = default;

  std::string getId() const {
    return id;
  }

  int getMaximumZoomLevel() const {
    return maximumZoomLevel;
  }

  std::string getName() const {
    return name;
  }

  std::vector<std::string> getServers() const {
    return servers;
  }

  bool isValid() const {
    return valid;
  }

  std::string getCopyright() const
  {
    return copyright;
  }

  static OnlineTileProvider fromJson(const nlohmann::json &obj);

private:
  bool valid{false};
  std::string id;
  std::string name;
  std::vector<std::string> servers;
  int maximumZoomLevel{-1};
  std::string copyright;
};

}

#endif /* OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H */
