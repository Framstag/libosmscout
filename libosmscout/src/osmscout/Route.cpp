/*
  This source is part of the libosmscout library
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

#include <osmscout/Route.h>
#include <osmscout/util/Number.h>

namespace osmscout {

  std::vector<FileOffset> Route::GetMemberOffsets() const
  {
    std::vector<FileOffset> offsets;
    for (const auto &segment:segments){
      for (const auto &member:segment.members){
        offsets.push_back(member.way);
      }
    }
    return offsets;
  }

  void Route::Read(const TypeConfig& typeConfig,
                   FileScanner& scanner)
  {
    fileOffset=scanner.GetPos();

    TypeId typeId=scanner.ReadTypeId(typeConfig.GetRouteTypeIdBytes());

    featureValueBuffer.SetType(typeConfig.GetRouteTypeInfo(typeId));

    featureValueBuffer.Read(scanner);

    bbox=scanner.ReadBox();

    // read segments
    uint32_t segmentCnt=scanner.ReadUInt32Number();
    uint64_t allMemberCnt=0;

    segments.clear();
    segments.reserve(segmentCnt);

    for (uint32_t i=0; i<segmentCnt; i++) {
      auto &segment=segments.emplace_back();
      uint32_t memberCnt=scanner.ReadUInt32Number();

      segment.members.reserve(memberCnt);
      allMemberCnt+=memberCnt;

      for (uint32_t j=0; j<memberCnt; j++) {
        auto &member=segment.members.emplace_back();
        member.way=scanner.ReadUInt64Number();
      }
    }

    // read directions
    std::vector<char> directionBuff(BitsToBytes(allMemberCnt));
    scanner.Read(directionBuff.data(), directionBuff.size());
    auto buffIt=directionBuff.begin();
    uint8_t bit=0x80;
    for (auto &segment : segments){
      for (auto &member : segment.members) {
        assert(buffIt != directionBuff.end());
        if (((*buffIt) & bit) != 0){
          member.direction=MemberDirection::backward;
        }
        if (bit==0x01){
          ++buffIt;
          bit=0x80;
        } else {
          bit=bit>>1;
        }
      }
    }

    nextFileOffset=scanner.GetPos();
  }

  void Route::Write(const TypeConfig& typeConfig,
                    FileWriter& writer) const
  {
    assert(featureValueBuffer.GetType()->IsRoute());
    writer.WriteTypeId(featureValueBuffer.GetType()->GetRouteId(),
                       typeConfig.GetRouteTypeIdBytes());

    featureValueBuffer.Write(writer);

    writer.WriteBox(bbox);

    // write segments
    assert(segments.size() <= std::numeric_limits<uint32_t>::max());
    writer.WriteNumber((uint32_t)segments.size());
    uint64_t memberCount=0;
    for (const auto &segment : segments){
      assert(segment.members.size() <= std::numeric_limits<uint32_t>::max());
      memberCount+=segment.members.size();
      writer.WriteNumber((uint32_t)segment.members.size());
      for (const auto &member : segment.members){
        static_assert(std::is_same<FileOffset, uint64_t>::value, "FileOffset should be uint64_t");
        writer.WriteNumber(member.way);
      }
    }

    // member direction stored as bits, 0 forward, 1 backward
    std::vector<char> directionBuff(BitsToBytes(memberCount));
    auto buffIt=directionBuff.begin();
    uint8_t bit=0x80;
    for (const auto &segment : segments){
      for (const auto &member : segment.members){
        assert(buffIt!=directionBuff.end());
        if (member.direction==MemberDirection::backward){
          (*buffIt)|=(char)bit;
        }
        if (bit==0x01){
          ++buffIt;
          bit=0x80;
        } else {
          bit=bit>>1;
        }
      }
    }
    writer.Write(directionBuff.data(), directionBuff.size());
  }
}
