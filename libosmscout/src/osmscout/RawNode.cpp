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

#include <osmscout/RawNode.h>

#include <cmath>

static double conversionFactor=10000000.0;

void RawNode::Read(std::istream& file)
{
  file.read((char*)&id,sizeof(id));

  if (!file) {
    return;
  }

  uint16_t tmpType;
  uint8_t  tagCount;
  uint32_t latValue;
  uint32_t lonValue;

  file.read((char*)&tmpType,sizeof(tmpType));
  file.read((char*)&latValue,sizeof(latValue));
  file.read((char*)&lonValue,sizeof(lonValue));

  type=(TypeId)tmpType;
  lat=latValue/conversionFactor-180.0;
  lon=lonValue/conversionFactor-90.0;

  file.read((char*)&tagCount,sizeof(tagCount));
  tags.resize(tagCount);
  for (size_t i=0; i<tagCount; i++) {
    file.read((char*)&tags[i].key,sizeof(tags[i].key));
    std::getline(file,tags[i].value,'\0');
  }
}

void RawNode::Write(std::ostream& file) const
{
  uint8_t  tagCount=tags.size();
  uint16_t tmpType=(uint16_t)type;
  uint32_t latValue=round((lat+180.0)*conversionFactor);
  uint32_t lonValue=round((lon+90.0)*conversionFactor);

  file.write((const char*)&id,sizeof(id));
  file.write((const char*)&tmpType,sizeof(tmpType));
  file.write((const char*)&latValue,sizeof(latValue));
  file.write((const char*)&lonValue,sizeof(lonValue));

  file.write((const char*)&tagCount,sizeof(tagCount));
  for (size_t i=0; i<tags.size(); i++) {
    file.write((const char*)&tags[i].key,sizeof(tags[i].key));
    file << tags[i].value << '\0';
  }
}

