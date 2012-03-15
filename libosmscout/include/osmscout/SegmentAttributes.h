#ifndef OSMSCOUT_SEGMENTATTRIBUTES_H
#define OSMSCOUT_SEGMENTATTRIBUTES_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/Progress.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

namespace osmscout {

  class OSMSCOUT_API SegmentAttributes
  {
  private:
    // Attribute availability flags (for optimized attribute storage)
    const static uint16_t hasMaxSpeed     = 1 << 10; //! We have maximum speed information stored on disk
    const static uint16_t hasTags         = 1 << 11; //! We have additional tags stored on disk
    const static uint16_t hasName         = 1 << 12; //! We have a name
    const static uint16_t hasRef          = 1 << 13; //! We have reference name
    const static uint16_t hasWidth        = 1 << 14; //! We have width
    const static uint16_t hasLayer        = 1 << 15; //! We have optional layer information

  public:
    // Common flags (Area & Way)
    const static uint16_t isArea          = 1 <<  0; //! We are an area (or a way if not set)
    const static uint16_t hasAccess       = 1 <<  1; //! We do have access rights to this way/area

    // Area flags
    const static uint16_t hasHouseNr      = 1 <<  2; //! We have a house number

    // Way flags
    const static uint16_t hasRestrictions = 1 <<  2; //! We have restrictions
    const static uint16_t isBridge        = 1 <<  3; //! We are a bridge
    const static uint16_t isTunnel        = 1 <<  4; //! We are a tunnel
    const static uint16_t startIsJoint    = 1 <<  5; //! Start node is a joint node
    const static uint16_t endIsJoint      = 1 <<  6; //! End node is a joint node
    const static uint16_t isOneway        = 1 <<  7; //! We are a oneway (in way direction)
    const static uint16_t isRoundabout    = 1 <<  8; //! We are a roundabout

  public:
    TypeId           type;     //! type of the way/relation
    mutable uint16_t flags;
    std::string      name;     //! name
    std::string      ref;      //! reference name (normally drawn in a plate)
    std::string      houseNr;  //! house number
    int8_t           layer;    //! layer to draw on
    uint8_t          width;    //! width of way
    uint8_t          maxSpeed; //! speed from 1..255km/h (0 means, not set)
    std::vector<Tag> tags;     //! list of preparsed tags

  public:
    inline SegmentAttributes()
    : type(typeIgnore),
      flags(0),
      layer(0),
      width(0),
      maxSpeed(0)
    {
      // no code
    }

    inline TypeId GetType() const
    {
      return type;
    }

    inline uint16_t GetFlags() const
    {
      return flags;
    }

    inline bool IsArea() const
    {
      return (flags & isArea)!=0;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetRefName() const
    {
      return ref;
    }

    inline std::string GetHouseNr() const
    {
      return houseNr;
    }

    inline int8_t GetLayer() const
    {
      return layer;
    }

    inline uint8_t GetWidth() const
    {
      return width;
    }

    inline uint8_t GetMaxSpeed() const
    {
      return maxSpeed;
    }

    inline bool IsBridge() const
    {
      return (flags & isBridge)!=0;
    }

    inline bool IsTunnel() const
    {
      return (flags & isTunnel)!=0;
    }

    inline bool IsOneway() const
    {
      return (flags & isOneway)!=0;
    }

    inline bool IsRoundabout() const
    {
      return (flags & isRoundabout)!=0;
    }

    inline bool HasAccess() const
    {
      return (flags & hasAccess)!=0;
    }

    inline bool StartIsJoint() const
    {
      return (flags & startIsJoint)!=0;
    }

    inline bool EndIsJoint() const
    {
      return (flags & endIsJoint)!=0;
    }

    inline bool HasRestrictions() const
    {
      return (flags & hasRestrictions)!=0;
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 Id id,
                 bool isArea,
                 std::vector<Tag>& tags,
                 bool& reverseNodes);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };
}

#endif
