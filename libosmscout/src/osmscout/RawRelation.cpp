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

bool RawRelation::Read(FileScanner& scanner)
{
  unsigned long tagCount;
  unsigned long memberCount;

  scanner.ReadNumber(id);
  scanner.ReadNumber(type);

  scanner.ReadNumber(tagCount);

  if (scanner.HasError()) {
    return false;
  }

  tags.resize(tagCount);
  for (size_t i=0; i<tagCount; i++) {
    scanner.ReadNumber(tags[i].key);
    scanner.Read(tags[i].value);
  }

  scanner.ReadNumber(memberCount);

  if (scanner.HasError()) {
    return false;
  }

  members.resize(memberCount);
  for (size_t i=0; i<memberCount; i++) {
    unsigned long memberType;

    scanner.ReadNumber(memberType);
    members[i].type=(MemberType)memberType;
    scanner.ReadNumber(members[i].id);
    scanner.Read(members[i].role);
  }

  return !scanner.HasError();
}

bool RawRelation::Write(FileWriter& writer) const
{
  writer.WriteNumber(id);
  writer.WriteNumber(type);

  writer.WriteNumber(tags.size());
  for (size_t i=0; i<tags.size(); i++) {
    writer.WriteNumber(tags[i].key);
    writer.Write(tags[i].value);
  }

  writer.WriteNumber(members.size());
  for (size_t i=0; i<members.size(); i++) {
    writer.WriteNumber(members[i].type);
    writer.WriteNumber(members[i].id);
    writer.Write(members[i].role);
  }

  return !writer.HasError();
}
