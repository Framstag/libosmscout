#ifndef OSMSCOUT_IMPORT_RAWRELATION_H
#define OSMSCOUT_IMPORT_RAWRELATION_H

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

#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class RawRelation : public Referencable
  {
  public:
    enum MemberType {
      memberNode,
      memberWay,
      memberRelation
    };

    struct Member
    {
      MemberType  type;
      Id          id;
      std::string role;
    };

  private:
    Id                  id;
    TypeId              type;

  public:
    std::vector<Tag>    tags;
    std::vector<Member> members;

  public:
    inline RawRelation()
    : type(typeIgnore)
    {
      // no code
    }

    inline Id GetId() const
    {
      return id;
    }

    inline TypeId GetType() const
    {
      return type;
    }

    void SetId(Id id);
    void SetType(TypeId type);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<RawRelation> RawRelationRef;
}

#endif
