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

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/AttributeAccess.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API WayAttributes
  {
  private:
    // Attribute availability flags (for optimized attribute storage)
    const static uint16_t hasNameAlt      = 1 <<  7; //! We have an alternative name (mainly in a second language)
    const static uint16_t hasName         = 1 <<  8; //! We have a name
    const static uint16_t hasRef          = 1 <<  9; //! We have reference name
    const static uint16_t hasHouseNr      = 1 << 10; //! We have a house number
    const static uint16_t hasLayer        = 1 << 11; //! We have optional layer information
    const static uint16_t hasWidth        = 1 << 12; //! We have width
    const static uint16_t hasMaxSpeed     = 1 << 13; //! We have maximum speed information
    const static uint16_t hasGrade        = 1 << 14; //! We have grade information
    const static uint16_t hasTags         = 1 << 15; //! We have additional tags

    const static uint16_t hasAccess       = 1 <<  0; //! We do have access rights to this way/area
    const static uint16_t isBridge        = 1 <<  1; //! We are a bridge
    const static uint16_t isTunnel        = 1 <<  2; //! We are a tunnel
    const static uint16_t isRoundabout    = 1 <<  4; //! We are a roundabout

  public:
    TypeId           type;     //! type of the way/relation
    std::string      name;     //! name

  private:
    mutable uint16_t flags;
    AttributeAccess  access;   //! Information regarding which vehicle can access this way
    std::string      nameAlt;  //! alternative name
    std::string      ref;      //! reference name (normally drawn in a plate)
    std::string      houseNr;  //! house number
    int8_t           layer;    //! layer to draw on
    uint8_t          width;    //! width of way
    uint8_t          maxSpeed; //! speed from 1..255km/h (0 means, not set)
    uint8_t          grade;    //! Quality of road/track 1 (good)...5 (bad)
    std::vector<Tag> tags;     //! list of preparsed tags

  public:
    inline WayAttributes()
    : type(typeIgnore),
      flags(0),
      layer(0),
      width(0),
      maxSpeed(0),
      grade(1)
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

    inline const AttributeAccess& GetAccess() const
    {
      return access;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetNameAlt() const
    {
      return nameAlt;
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

    inline uint8_t GetGrade() const
    {
      return grade;
    }

    inline bool IsBridge() const
    {
      return (flags & isBridge)!=0;
    }

    inline bool IsTunnel() const
    {
      return (flags & isTunnel)!=0;
    }

    inline bool IsRoundabout() const
    {
      return (flags & isRoundabout)!=0;
    }

    inline bool HasAccess() const
    {
      return (flags & hasAccess)!=0;
    }

    inline bool HasTags() const
    {
      return !tags.empty();
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 Id id,
                 std::vector<Tag>& tags,
                 bool& reverseNodes);

    void SetLayer(int8_t layer);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;

    bool operator==(const WayAttributes& other) const;
    bool operator!=(const WayAttributes& other) const;
  };

  class OSMSCOUT_API Way : public Referencable
  {
  private:
    FileOffset            fileOffset;
    WayAttributes         attributes;

  public:
    std::vector<Id>       ids;
    std::vector<GeoCoord> nodes;

  public:
    inline Way()
    : fileOffset(0)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline const WayAttributes& GetAttributes() const
    {
      return attributes;
    }

    inline TypeId GetType() const
    {
      return attributes.GetType();
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

    inline uint8_t GetMaxSpeed() const
    {
      return attributes.GetMaxSpeed();
    }

    inline uint8_t GetGrade() const
    {
      return attributes.GetGrade();
    }

    inline bool IsBridge() const
    {
      return attributes.IsBridge();
    }

    inline bool IsTunnel() const
    {
      return attributes.IsTunnel();
    }

    inline bool IsRoundabout() const
    {
      return attributes.IsRoundabout();
    }

    inline bool HasAccess() const
    {
      return attributes.HasAccess();
    }

    inline bool HasTags() const
    {
      return !attributes.GetTags().empty();
    }

    inline size_t GetTagCount() const
    {
      return attributes.GetTags().size();
    }

    inline TagId GetTagKey(size_t idx) const
    {
      return attributes.GetTags()[idx].key;
    }

    inline const std::string& GetTagValue(size_t idx) const
    {
      return attributes.GetTags()[idx].value;
    }

    bool GetCenter(double& lat,
                   double& lon) const;
    void GetBoundingBox(double& minLon,
                        double& maxLon,
                        double& minLat,
                        double& maxLat) const;
    void GetCoordinates(size_t nodeIndex,
                        double& lat,
                        double& lon) const;

    void SetType(TypeId type);

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 Id id,
                 std::vector<Tag>& tags,
                 bool& reverseNodes);

    void SetLayerToMax();

    bool Read(FileScanner& scanner);
    bool ReadOptimized(FileScanner& scanner);

    bool Write(FileWriter& writer) const;
    bool WriteOptimized(FileWriter& writer) const;
  };

  typedef Ref<Way> WayRef;
}

#endif
