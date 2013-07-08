#ifndef OSMSCOUT_IMPORT_RAWWAY_H
#define OSMSCOUT_IMPORT_RAWWAY_H

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

#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class RawWay : public Referencable
  {
  private:
    // Attribute availability flags (for optimized attribute storage)

    const static uint8_t isArea  = 1 <<  0; //! We are an area
    const static uint8_t hasType = 1 <<  1; //! We have a type
    const static uint8_t hasTags = 1 <<  2; //! We have tags

  private:
    OSMId              id;
    TypeId             type;
    mutable uint8_t    flags;
    std::vector<Tag>   tags;
    std::vector<OSMId> nodes;

  public:

  public:
    inline RawWay()
    : id(0),
      type(typeIgnore),
      flags(0)
    {
      // no code
    }

    inline OSMId GetId() const
    {
      return id;
    }

    inline TypeId GetType() const
    {
      return type;
    }

    inline bool IsArea() const
    {
      return (flags & isArea)!=0;
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    inline const std::vector<OSMId>& GetNodes() const
    {
      return nodes;
    }

    inline size_t GetNodeCount() const
    {
      return nodes.size();
    }

    inline OSMId GetNodeId(size_t idx) const
    {
      return nodes[idx];
    }

    inline OSMId GetFirstNodeId() const
    {
      return nodes[0];
    }

    inline OSMId GetLastNodeId() const
    {
      return nodes[nodes.size()-1];
    }

    void SetId(OSMId id);
    void SetType(TypeId type, bool area);
    void SetTags(const std::vector<Tag>& tags);
    void SetNodes(const std::vector<OSMId>& nodes);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<RawWay> RawWayRef;
}

#endif
