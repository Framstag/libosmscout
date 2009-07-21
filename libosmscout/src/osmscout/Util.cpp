/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/Util.h>

void GetKeysForName(const std::string& name, std::set<uint32_t>& keys)
{
  for (size_t s=0; s==0 || s+4<=name.length(); s++) {
    uint32_t value=0;

    if (name.length()>s) {
      value=name[s];
    }
    value=value << 8;

    if (name.length()>s+1) {
      value+=name[s+1];
    }
    value=value << 8;

    if (name.length()>s+2) {
      value+=name[s+2];
    }
    value=value << 8;

    if (name.length()>s+3) {
      value+=name[s+3];
    }

    keys.insert(value);
  }
}



