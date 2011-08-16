/*
  This source is part of the libosmscout library
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

#include <limits>

namespace osmscout {

  void RawRelation::SetId(Id id)
  {
    this->id=id;
  }

  void RawRelation::SetType(TypeId type)
  {
    this->type=type;
  }

  bool RawRelation::Read(FileScanner& scanner)
  {
    uint32_t tagCount;
    uint32_t memberCount;

    scanner.Read(id);
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

    if (memberCount>0) {
      Id minId;

      if (!scanner.Read(minId)) {
        return false;
      }

      for (size_t i=0; i<memberCount; i++) {
        uint32_t memberType;
        Id       id;

        scanner.ReadNumber(memberType);
        members[i].type=(MemberType)memberType;

        scanner.ReadNumber(id);
        members[i].id=minId+id;

        scanner.Read(members[i].role);
      }
    }

    return !scanner.HasError();
  }

  bool RawRelation::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber(type);

    writer.WriteNumber((uint32_t)tags.size());
    for (size_t i=0; i<tags.size(); i++) {
      writer.WriteNumber(tags[i].key);
      writer.Write(tags[i].value);
    }

    writer.WriteNumber((uint32_t)members.size());

    if (!members.empty()) {
      Id minId=std::numeric_limits<Id>::max();

      for (size_t i=0; i<members.size(); i++) {
        minId=std::min(minId,members[i].id);
      }

      writer.Write(minId);

      for (size_t i=0; i<members.size(); i++) {
        writer.WriteNumber(members[i].type);
        writer.WriteNumber(members[i].id-minId);
        writer.Write(members[i].role);
      }
    }

    return !writer.HasError();
  }
}

