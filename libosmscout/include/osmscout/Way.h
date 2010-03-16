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

#include <osmscout/FileReader.h>
#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Point.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  class Way
  {
  public:
    enum RestrictionType
    {
      rstrAllowTurn  = 0, //! 0th member will be to, rest via ids
      rstrForbitTurn = 1  //! 0th member will be to, rest via ids
    };

    struct Restriction
    {
      RestrictionType type;
      std::vector<Id> members;
    };

  public:
    // Common flags
    const static uint16_t isArea          = 1 <<  0; //! We are an area
    const static uint16_t hasTags         = 1 <<  1; //! We have additional tags store on disk
    const static uint16_t hasName         = 1 <<  2; //! We have a name
    const static uint16_t hasRef          = 1 <<  3; //! We have reference name
    const static uint16_t hasRestrictions = 1 <<  4; //! We have restrictions

    // Area flags
    const static uint16_t isBuilding      = 1 << 15; //! We are a building

    // Way flags
    const static uint16_t hasLayer        = 1 << 10; //! We have optional layer information
    const static uint16_t isBridge        = 1 << 11; //! We are a bridge
    const static uint16_t isTunnel        = 1 << 12; //! We are a tunnel
    const static uint16_t startIsJoint    = 1 << 13; //! Start node is a joint node
    const static uint16_t endIsJoint      = 1 << 14; //! End node is a joint node
    const static uint16_t isOneway        = 1 << 15; //! We are a oneway (in way direction)

  public:
    Id                        id;
    TypeId                    type;
    uint16_t                  flags;
    std::vector<Point>        nodes;
    std::string               name;
    std::string               ref;
    int8_t                    layer;
    std::vector<Tag>          tags;
    std::vector<Restriction>  restrictions;

  public:
    inline Way()
    : type(typeIgnore),
      flags(0),
      layer(0)
    {
      // no code
    }

    inline bool IsArea() const
    {
      return flags & isArea;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetRefName() const
    {
      return ref;
    }

    inline bool IsOneway() const
    {
      return flags & isOneway;
    }

    bool GetCenter(double& lat, double& lon) const;

    bool Read(FileReader& reader);
    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };
}

#endif
