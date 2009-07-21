#ifndef OSMSCOUT_WAY_H
#define OSMSCOUT_WAY_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
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

#include <fstream>

#include <osmscout/Point.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

class Way
{
public:
  // Common flags
  const static uint8_t isArea       = 1 << 0; //! We are an area

  // Area flags
  const static uint8_t isBuilding   = 1 << 1; //! We are a building

  // Way flags
  const static uint8_t hasLayer     = 1 << 1; //! We have optional layer information
  const static uint8_t isBridge     = 1 << 2; //! We are a bridge
  const static uint8_t isTunnel     = 1 << 3; //! We are a tunnel
  const static uint8_t startIsJoint = 1 << 4; //! Start node is a joint node
  const static uint8_t endIsJoint   = 1 << 5; //! End node is a joint node
  const static uint8_t isOneway     = 1 << 6; //! We are a oneway (in way direction)

public:
  Id                 id;
  TypeId             type;
  uint8_t            flags;
  int8_t             layer;
  std::vector<Tag>   tags;
  std::vector<Point> nodes;

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

  inline bool IsOneway() const
  {
    return flags & isOneway;
  }

  void Read(std::istream& file);
  void Write(std::ostream& file) const;

  std::string GetName() const;
  std::string GetRefName() const;
};

#endif
