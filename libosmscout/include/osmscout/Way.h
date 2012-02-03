#ifndef OSMSCOUT_WAY_H
#define OSMSCOUT_WAY_H

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

#include <osmscout/Point.h>
#include <osmscout/SegmentAttributes.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API Way : public Referencable
  {
  public:
    enum RestrictionType
    {
      rstrAllowTurn  = 0, //! 0th member will be to, rest via ids
      rstrForbitTurn = 1  //! 0th member will be to, rest via ids
    };

    struct OSMSCOUT_API Restriction
    {
      RestrictionType type;
      std::vector<Id> members;
    };

  private:
    Id                        id;
    SegmentAttributes         attributes;

  public:
    std::vector<Point>        nodes;
    std::vector<Restriction>  restrictions;

  public:
    inline Way()
    {
      // no code
    }

    inline Id GetId() const
    {
      return id;
    }

    inline const SegmentAttributes& GetAttributes() const
    {
      return attributes;
    }

    inline TypeId GetType() const
    {
      return attributes.GetType();
    }

    inline bool IsArea() const
    {
      return attributes.IsArea();
    }

    inline std::string GetName() const
    {
      return attributes.GetName();
    }

    inline std::string GetRefName() const
    {
      return attributes.GetRefName();
    }

    inline std::string GetHouseNr() const
    {
      return attributes.GetHouseNr();
    }

    inline int8_t GetLayer() const
    {
      return attributes.GetLayer();
    }

    inline uint8_t GetWidth() const
    {
      return attributes.GetWidth();
    }

    inline bool IsBridge() const
    {
      return attributes.IsBridge();
    }

    inline bool IsTunnel() const
    {
      return attributes.IsTunnel();
    }

    inline bool IsOneway() const
    {
      return attributes.IsOneway();
    }

    inline bool StartIsJoint() const
    {
      return attributes.StartIsJoint();
    }

    inline bool EndIsJoint() const
    {
      return attributes.EndIsJoint();
    }

    inline size_t GetTagCount() const
    {
      return attributes.tags.size();
    }

    inline TagId GetTagKey(size_t idx) const
    {
      return attributes.tags[idx].key;
    }

    inline const std::string& GetTagValue(size_t idx) const
    {
      return attributes.tags[idx].value;
    }

    bool GetCenter(double& lat,
                   double& lon) const;
    void GetBoundingBox(double& minLon,
                        double& maxLon,
                        double& minLat,
                        double& maxLat) const;

    void SetId(Id id);
    void SetType(TypeId type);

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 bool isArea,
                 std::vector<Tag>& tags,
                 bool& reverseNodes);

    void SetRestrictions(const std::vector<Way::Restriction>& restrictions);

    void SetStartIsJoint(bool isJoint);
    void SetEndIsJoint(bool isJoint);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<Way> WayRef;
}

#endif
