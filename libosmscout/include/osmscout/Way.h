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

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Point.h>
#include <osmscout/SegmentAttributes.h>
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
    Id                        id;
    SegmentAttributes         attributes;
    std::vector<Point>        nodes;
    std::vector<Restriction>  restrictions;

  public:
    inline Way()
    {
      // no code
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

    inline int8_t GetLayer() const
    {
      return attributes.GetLayer();
    }

    inline uint8_t GetWidth() const
    {
      return attributes.GetWidth();
    }

    inline bool IsBuilding() const
    {
      return attributes.IsBuilding();
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


    bool GetCenter(double& lat, double& lon) const;

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };
}

#endif
