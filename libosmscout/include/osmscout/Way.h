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
  enum RestrictionType
  {
    rstrAllowTurn  = 0,
    rstrForbitTurn = 1
  };

  class Restriction
  {
  public:
    RestrictionType type;

    Restriction(RestrictionType type);

    inline RestrictionType GetType() const
    {
      return type;
    }
  };

  class AllowTurnRestriction : public Restriction
  {
  public:
    Id via;
    Id to;

  public:
    AllowTurnRestriction(Id via, Id to);

    inline Id GetVia() const
    {
      return via;
    }

    inline Id GetTo() const
    {
      return to;
    }
  };

  class ForbitTurnRestriction : public Restriction
  {
  public:
    Id via;
    Id to;

  public:
    ForbitTurnRestriction(Id via, Id to);

    inline Id GetVia() const
    {
      return via;
    }

    inline Id GetTo() const
    {
      return to;
    }
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
  std::vector<Restriction*> restrictions;

public:
  inline Way()
  : type(typeIgnore),
    flags(0),
    layer(0)
  {
    // no code
  }

  inline Way(const Way& other)
  {
    if (this==&other) {
      return;
    }

    id=other.id;
    type=other.type;
    flags=other.flags;
    nodes=other.nodes;
    name=other.name;
    ref=other.ref;
    layer=other.layer;
    tags=other.tags;

    if (other.restrictions.size()>0) {
      restrictions.resize(other.restrictions.size());

      for (size_t i=0; i<other.restrictions.size(); i++) {
        if (other.restrictions[i]->type==rstrAllowTurn) {
          AllowTurnRestriction *r=static_cast<AllowTurnRestriction*>(other.restrictions[i]);

          restrictions[i]=new AllowTurnRestriction(r->via,r->to);
        }
        else if (other.restrictions[i]->type==rstrForbitTurn) {
          ForbitTurnRestriction *r=static_cast<ForbitTurnRestriction*>(other.restrictions[i]);

          restrictions[i]=new ForbitTurnRestriction(r->via,r->to);
        }
      }
    }
  }

  inline const Way& operator=(const Way& other)
  {
    if (this==&other) {
      return *this;
    }

    id=other.id;
    type=other.type;
    flags=other.flags;
    nodes=other.nodes;
    name=other.name;
    ref=other.ref;
    layer=other.layer;
    tags=other.tags;

    if (other.restrictions.size()>0) {
      restrictions.resize(other.restrictions.size());

      for (size_t i=0; i<other.restrictions.size(); i++) {
        if (other.restrictions[i]->type==rstrAllowTurn) {
          AllowTurnRestriction *r=static_cast<AllowTurnRestriction*>(other.restrictions[i]);

          restrictions[i]=new AllowTurnRestriction(r->via,r->to);
        }
        else if (other.restrictions[i]->type==rstrForbitTurn) {
          ForbitTurnRestriction *r=static_cast<ForbitTurnRestriction*>(other.restrictions[i]);

          restrictions[i]=new ForbitTurnRestriction(r->via,r->to);
        }
      }
    }

    return *this;
  }

  inline ~Way()
  {
    for (size_t i=0; i<restrictions.size(); i++) {
      delete restrictions[i];
    }
  };

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

  void Read(std::istream& file);
  void Write(std::ostream& file) const;
};

#endif
