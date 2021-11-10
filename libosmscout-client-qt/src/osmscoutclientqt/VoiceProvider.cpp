/*
  This source is part of the libosmscout-map library
  Copyright (C) 2020  Lukáš Karas

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

#include <osmscoutclientqt/VoiceProvider.h>

namespace osmscout {

VoiceProvider VoiceProvider::fromJson(QJsonValue val)
{
  if (!val.isObject())
    return VoiceProvider();

  QJsonObject obj = val.toObject();
  auto name = obj["name"];
  auto uri = obj["uri"];
  auto listUri = obj["listUri"];

  if (!(name.isString() && uri.isString() && listUri.isString())){
    return VoiceProvider();
  }
  return VoiceProvider(name.toString(), uri.toString(), listUri.toString());
}
}
