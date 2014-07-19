#ifndef OSMSCOUT_IMPORT_RAWNODE_H
#define OSMSCOUT_IMPORT_RAWNODE_H

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

#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class RawNode : public Referencable
  {
  private:
    OSMId            id;
    TypeInfoRef      type;
    GeoCoord         coords;
    std::vector<Tag> tags;

  public:
    inline RawNode()
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
      return type;
    }

    inline TypeId GetTypeId() const
    {
      return type->GetId();
    }

    inline const GeoCoord& GetCoords() const
    {
      return coords;
    }

    inline double GetLat() const
    {
      return coords.GetLat();
    }

    inline double GetLon() const
    {
      return coords.GetLon();
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    inline bool IsIdentical(const RawNode& other) const
    {
      return id==other.id;
    }

    inline bool IsSame(const RawNode& other) const
    {
      return coords==other.coords;
    }

    inline bool IsEqual(const RawNode& other) const
    {
      return id==other.id || coords==other.coords;
    }

    void SetId(OSMId id);
    void SetType(const TypeInfoRef& type);
    void SetCoords(double lon, double lat);
    void SetTags(const std::vector<Tag>& tags);

    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    bool Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef Ref<RawNode> RawNodeRef;
}

#endif
