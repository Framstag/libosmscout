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
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API WayAttributes
  {
  private:
    // Attribute availability flags (for optimized attribute storage)
    static const uint16_t hasNameAlt      = 1 <<  8; //! We have an alternative name (mainly in a second language)
    static const uint16_t hasName         = 1 <<  9; //! We have a name
    static const uint16_t hasRef          = 1 << 10; //! We have reference name
    static const uint16_t hasLayer        = 1 << 11; //! We have optional layer information
    static const uint16_t hasWidth        = 1 << 12; //! We have width
    static const uint16_t hasMaxSpeed     = 1 << 13; //! We have maximum speed information
    static const uint16_t hasGrade        = 1 << 14; //! We have grade information
    static const uint16_t hasTags         = 1 << 15; //! We have additional tags

    static const uint16_t isBridge        = 1 <<  0; //! We are a bridge
    static const uint16_t isTunnel        = 1 <<  1; //! We are a tunnel
    static const uint16_t isRoundabout    = 1 <<  2; //! We are a roundabout

  private:
    TypeId           type;     //! type of the way/relation
    mutable uint16_t flags;
    std::string      name;     //! name
    std::string      nameAlt;  //! alternative name
    AttributeAccess  access;   //! Information regarding which vehicle can access this way
    std::string      ref;      //! reference name (normally drawn in a plate)
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

    inline bool HasTags() const
    {
      return !tags.empty();
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    inline std::vector<Tag>& GetTags()
    {
      return tags;
    }

    void SetType(TypeId type);

    void SetFeatures(const TypeConfig& typeConfig,
                     const FeatureValueBuffer& buffer);

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

    inline WayAttributes& GetAttributes()
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

    inline bool IsCircular() const
    {
      return attributes.IsRoundabout() ||
          (ids[0]!=0 && ids[0]==ids[ids.size()-1]);
    }

    inline void GetBoundingBox(double& minLon,
                               double& maxLon,
                               double& minLat,
                               double& maxLat) const
    {
      osmscout::GetBoundingBox(nodes,
                               minLon,
                               maxLon,
                               minLat,
                               maxLat);
    }

    bool GetCenter(double& lat,
                   double& lon) const;
    void GetCoordinates(size_t nodeIndex,
                        double& lat,
                        double& lon) const;

    bool GetNodeIndexByNodeId(Id id,
                              size_t& index) const;

    void SetType(TypeId type);

    void SetFeatures(const TypeConfig& typeConfig,
                     const FeatureValueBuffer& buffer);

    void SetLayerToMax();

    bool Read(FileScanner& scanner);
    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    bool ReadOptimized(FileScanner& scanner);

    bool Write(FileWriter& writer) const;
    bool Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
    bool WriteOptimized(FileWriter& writer) const;
  };

  typedef Ref<Way> WayRef;
}

#endif
