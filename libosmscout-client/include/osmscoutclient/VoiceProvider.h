#ifndef OSMSCOUT_CLIENT_VOICEPROVIDER_H
#define OSMSCOUT_CLIENT_VOICEPROVIDER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020 Lukas Karas

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

#include <string>

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/json/json_fwd.hpp>

#include <osmscout/util/String.h>

namespace osmscout {

/**
 * \ingroup ClientAPI
 */
struct OSMSCOUT_CLIENT_API VoiceProvider
{
private:
  bool valid=false;
  std::string uri;
  std::string listUri;
  std::string name;

public:
  VoiceProvider() = default;
  VoiceProvider(const VoiceProvider &) = default;
  VoiceProvider(VoiceProvider &&) = default;

  VoiceProvider(const std::string &name, const std::string &uri, const std::string &listUri):
    valid(true), uri(uri), listUri(listUri), name(name) {}

  virtual ~VoiceProvider() = default;

  VoiceProvider& operator=(const VoiceProvider&) = default;
  VoiceProvider& operator=(VoiceProvider&&) = default;

  std::string getName() const
  {
    return name;
  }

  std::string getUri() const
  {
    return uri;
  }

  std::string getListUri(const std::string &locale="en") const
  {
    return ReplaceString(listUri, "%1", locale);
  }

  bool isValid() const
  {
    return valid;
  }

  static VoiceProvider fromJson(const nlohmann::json &obj);
};

}

#endif // OSMSCOUT_CLIENT_VOICEPROVIDER_H
