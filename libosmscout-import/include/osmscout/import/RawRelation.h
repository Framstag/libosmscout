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

#include <memory>
#include <unordered_map>

#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/TagErrorReporter.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class RawRelation CLASS_FINAL
  {
  public:
    enum MemberType {
      memberNode      = 0,
      memberWay       = 1,
      memberRelation  = 2
    };

    struct Member
    {
      MemberType  type;
      OSMId       id;
      std::string role;
    };

  private:
    OSMId               id;
    FeatureValueBuffer  featureValueBuffer;

  public:
    std::vector<Member> members;

  public:
    inline RawRelation()
    : id(0)
    {
      // no code
    }

    inline OSMId GetId() const
    {
      return id;
    }

    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    inline size_t GetFeatureCount() const
    {
      return featureValueBuffer.GetType()->GetFeatureCount();
    }

    inline bool HasFeature(size_t idx) const
    {
      return featureValueBuffer.HasFeature(idx);
    }

    inline const FeatureInstance& GetFeature(size_t idx) const
    {
      return featureValueBuffer.GetType()->GetFeature(idx);
    }

    inline FeatureValue* GetFeatureValue(size_t idx) const
    {
      return featureValueBuffer.GetValue(idx);
    }

    inline const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return featureValueBuffer;
    }

    void SetId(OSMId id);
    void SetType(const TypeInfoRef& type);

    void Parse(TagErrorReporter& errorReporter,
               const TypeConfig& typeConfig,
               const TagMap& tags);
    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef std::shared_ptr<RawRelation> RawRelationRef;
}

#endif
