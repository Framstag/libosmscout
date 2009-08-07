#ifndef OSMSCOUT_UTIL_H
#define OSMSCOUT_UTIL_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <stdint.h>

#include <set>
#include <string>

extern void GetKeysForName(const std::string& name, std::set<uint32_t>& keys);

extern bool EncodeNumber(unsigned long number,
                         size_t bufferLength,
                         char* buffer,
                         size_t& bytes);

extern bool DecodeNumber(const char* buffer, unsigned long& number, size_t& bytes);

#endif
