#ifndef OSMSCOUT_IMPORT_RAWCOASTLINE_H
#define OSMSCOUT_IMPORT_RAWCOASTLINE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

  class RawCoastline : public Referencable
  {
  private:
    // Attribute availability flags (for optimized attribute storage)

    static const uint8_t isArea  = 1 <<  0; //! We are an area

  private:
    OSMId               id;
    mutable uint8_t     flags;
    std::vector<OSMId>  nodes;

  public:

  public:
    inline RawCoastline()
    : flags(0)
    {
      // no code
    }

    inline OSMId GetId() const
    {
      return id;
    }

    inline bool IsArea() const
    {
      return (flags & isArea)!=0;
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

    void SetId(OSMId id);
    void SetType(bool area);
    void SetNodes(const std::vector<OSMId>& nodes);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<RawCoastline> RawCoastlineRef;
}

#endif
