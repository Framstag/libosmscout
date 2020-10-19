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

namespace osmscout {

  ObjectOSMRef RawRelation::Member::GetObjectOSMRef() const
  {
    if (type==MemberType::memberNode) {
      return ObjectOSMRef(id,osmRefNode);
    }

    if (type==MemberType::memberWay) {
      return ObjectOSMRef(id,osmRefWay);
    }

    if (type==MemberType::memberRelation) {
      return ObjectOSMRef(id,osmRefRelation);
    }

    return ObjectOSMRef();
  }

  void RawRelation::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawRelation::SetType(const TypeInfoRef& type)
  {
    featureValueBuffer.SetType(type);
  }

  void RawRelation::Parse(TagErrorReporter& errorReporter,
                          const TagRegistry& tagRegistry,
                          const TagMap& tags)
  {
    ObjectOSMRef object(id,
                        osmRefRelation);

    featureValueBuffer.Parse(errorReporter,
                             tagRegistry,
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

    id=scanner.ReadInt64Number();

    uint32_t tmpType=scanner.ReadUInt32Number();

    TypeInfoRef type=typeConfig.GetTypeInfo(tmpType);

    featureValueBuffer.SetType(type);

    if (!type->GetIgnore()) {
      featureValueBuffer.Read(scanner);
    }

    uint32_t memberCount=scanner.ReadUInt32Number();

    if (memberCount>0) {
      OSMId minId=scanner.ReadInt64Number();

      members.resize(memberCount);

      for (auto& member : members) {
        uint32_t memberType=scanner.ReadUInt32Number();

        member.type=(MemberType)memberType;

        OSMId    id=scanner.ReadInt64Number();
        member.id=minId+id;

        member.role=scanner.ReadString();
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

    for (const auto& member : members) {
      writer.WriteNumber((uint32_t) member.type);
      writer.WriteNumber(member.id-minId);
      writer.Write(member.role);
    }
  }
}

