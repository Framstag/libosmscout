#ifndef OSMSCOUT_AREA_H
#define OSMSCOUT_AREA_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API AreaAttributes
  {
  private:
    // Attribute availability flags (for optimized attribute storage)
    const static uint8_t hasNameAlt      = 1 << 4; //! We have an alternative name (mainly in a second language)
    const static uint8_t hasName         = 1 << 5; //! We have a name
    const static uint8_t hasHouseNr      = 1 << 6; //! We have a house number
    const static uint8_t hasTags         = 1 << 7; //! We have additional tags

    const static uint8_t hasAccess       = 1 << 0; //! We do have access rights to this way/area

  public:
    TypeId           type;     //! type of the way/relation
    std::string      name;     //! name

  private:
    mutable uint8_t  flags;
    std::string      nameAlt;  //! alternative name
    std::string      houseNr;  //! house number
    std::vector<Tag> tags;     //! list of preparsed tags

  public:
    inline AreaAttributes()
    : type(typeIgnore),
      flags(0)
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

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetNameAlt() const
    {
      return nameAlt;
    }

    inline std::string GetHouseNr() const
    {
      return houseNr;
    }

    inline bool HasAccess() const
    {
      return (flags & hasAccess)!=0;
    }

    inline const std::vector<Tag>& GetTags() const
    {
      return tags;
    }

    bool SetTags(Progress& progress,
                 const TypeConfig& typeConfig,
                 std::vector<Tag>& tags);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;

    bool operator==(const AreaAttributes& other) const;
    bool operator!=(const AreaAttributes& other) const;
  };

  /**
    Representation of an (complex/multipolygon) area
    */
  class OSMSCOUT_API Area : public Referencable
  {
  public:
    class Role
    {
    public:
      AreaAttributes        attributes;
      uint8_t               ring;
      std::vector<Id>       ids;
      std::vector<GeoCoord> nodes;

    public:
      inline const AreaAttributes& GetAttributes() const
      {
        return attributes;
      }

      inline TypeId GetType() const
      {
        return attributes.GetType();
      }

      inline uint16_t GetFlags() const
      {
        return attributes.GetFlags();
      }

      inline std::string GetName() const
      {
        return attributes.GetName();
      }
    };

  private:
    FileOffset        fileOffset;

  public:
    AreaAttributes    attributes;
    std::vector<Role> roles;

  public:
    inline Area()
    : fileOffset(0)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline const AreaAttributes& GetAttributes() const
    {
      return attributes;
    }

    inline TypeId GetType() const
    {
      return attributes.GetType();
    }

    inline uint16_t GetFlags() const
    {
      return attributes.GetFlags();
    }

    inline std::string GetName() const
    {
      return attributes.GetName();
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

    void SetType(TypeId type);

    bool Read(FileScanner& scanner);
    bool ReadOptimized(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
    bool WriteOptimized(FileWriter& writer) const;
  };

  typedef Ref<Area> AreaRef;
}

#endif
