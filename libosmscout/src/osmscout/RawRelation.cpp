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

#include <osmscout/RawRelation.h>

void RawRelation::Read(std::istream& file)
{
  uint8_t  tagCount;
  uint16_t memberCount;

  file.read((char*)&id,sizeof(id));

  if (!file) {
    return;
  }

  file.read((char*)&type,sizeof(type));

  file.read((char*)&tagCount,sizeof(tagCount));
  tags.resize(tagCount);
  for (size_t i=0; i<tagCount; i++) {
    file.read((char*)&tags[i].key,sizeof(tags[i].key));
    std::getline(file,tags[i].value,'\0');
  }

  file.read((char*)&memberCount,sizeof(memberCount));
  members.resize(memberCount);
  for (size_t i=0; i<memberCount; i++) {
    file.read((char*)&members[i].type,sizeof(members[i].type));
    file.read((char*)&members[i].id,sizeof(members[i].id));
    std::getline(file,members[i].role,'\0');
  }
}

void RawRelation::Write(std::ostream& file) const
{
  uint8_t  tagCount=tags.size();
  uint16_t memberCount=members.size();

  file.write((const char*)&id,sizeof(id));
  file.write((const char*)&type,sizeof(type));

  file.write((const char*)&tagCount,sizeof(tagCount));
  for (size_t i=0; i<tags.size(); i++) {
    file.write((const char*)&tags[i].key,sizeof(tags[i].key));
    file << tags[i].value << '\0';
  }

  file.write((const char*)&memberCount,sizeof(memberCount));
  for (size_t i=0; i<members.size(); i++) {
    file.write((const char*)&members[i].type,sizeof(members[i].type));
    file.write((const char*)&members[i].id,sizeof(members[i].id));
    file << members[i].role << '\0';
  }
}

