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

#include <osmscout/import/RawRelation.h>

#include <algorithm>
#include <limits>

namespace osmscout {

  void RawRelation::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawRelation::SetType(const TypeInfoRef& type)
  {
    featureValueBuffer.SetType(type);
  }

  void RawRelation::Parse(TagErrorReporter& errorReporter,
                          const TypeConfig& typeConfig,
                          const TagMap& tags)
  {
    ObjectOSMRef object(id,
                        osmRefRelation);

    featureValueBuffer.Parse(errorReporter,
                             typeConfig,
                             object,
                             tags);
  }

  /**
   * Reads data from the given FileScanner
   *
   * @throws IOException
   */
  void RawRelation::Read(const TypeConfig& typeConfig,
                         FileScanner& scanner)
  {
    uint32_t memberCount;

    scanner.ReadNumber(id);

    uint32_t tmpType;

    scanner.ReadNumber(tmpType);

    TypeInfoRef type=typeConfig.GetTypeInfo(tmpType);

    featureValueBuffer.SetType(type);

    if (!type->GetIgnore()) {
      featureValueBuffer.Read(scanner);
    }

    scanner.ReadNumber(memberCount);

    members.resize(memberCount);

    if (memberCount>0) {
      OSMId minId;

      scanner.ReadNumber(minId);

      for (size_t i=0; i<memberCount; i++) {
        uint32_t memberType;
        OSMId    id;

        scanner.ReadNumber(memberType);
        members[i].type=(MemberType)memberType;

        scanner.ReadNumber(id);
        members[i].id=minId+id;

        scanner.Read(members[i].role);
      }
    }
  }

  /**
   * Writes data to the given FileWriter
   *
   * @throws IOException
   */
  void RawRelation::Write(const TypeConfig& /*typeConfig*/,
                          FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteNumber((uint32_t)featureValueBuffer.GetType()->GetIndex());

    if (!featureValueBuffer.GetType()->GetIgnore()) {
      featureValueBuffer.Write(writer);
    }

    writer.WriteNumber((uint32_t)members.size());

    assert(!members.empty());

    OSMId minId=members[0].id;

    for (size_t i=1; i<members.size(); i++) {
      minId=std::min(minId,members[i].id);
    }

    writer.WriteNumber(minId);

    for (size_t i=0; i<members.size(); i++) {
      writer.WriteNumber((uint32_t)members[i].type);
      writer.WriteNumber(members[i].id-minId);
      writer.Write(members[i].role);
    }
  }
}

