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
    const static uint8_t isSimple        = 1 << 2; //1 We are a simple area, only one Ring, no roles
    const static uint8_t hasNameAlt      = 1 << 3; //! We have an alternative name (mainly in a second language)
    const static uint8_t hasName         = 1 << 4; //! We have a name
    const static uint8_t hasStreet       = 1 << 5; //! Street and..
    const static uint8_t hasHouseNr      = 1 << 6; //! ...house number
    const static uint8_t hasTags         = 1 << 7; //! We have additional tags

    const static uint8_t hasAccess       = 1 << 0; //! We do have (general) access rights to this way/area

  public:
    std::string      name;     //! name

  private:
    mutable uint8_t  flags;
    std::string      nameAlt;  //! alternative name
    std::string      street;   //! Street and...
    std::string      houseNr;  //! ...house number
    std::vector<Tag> tags;     //! list of preparsed tags

  private:
    void GetFlags(uint8_t& flags) const;
    bool Read(FileScanner& scanner);
    bool Read(FileScanner& scanner,
              uint8_t flags);
    bool Write(FileWriter& writer) const;
    bool Write(FileWriter& writer,
               uint8_t flags) const;

    friend class Area;

  public:
    inline AreaAttributes()
    : flags(0)
    {
      // no code
    }

    inline uint8_t GetFlags() const
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

    inline std::string GetStreet() const
    {
      return street;
    }

    inline std::string GetHouseNr() const
    {
      return houseNr;
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
                 std::vector<Tag>& tags);

    bool operator==(const AreaAttributes& other) const;
    bool operator!=(const AreaAttributes& other) const;
  };

  /**
    Representation of an (complex/multipolygon) area
    */
  class OSMSCOUT_API Area : public Referencable
  {
  public:
    const static size_t masterRingId = 0;
    const static size_t outerRingId = 1;

  public:
    class Ring
    {
    public:
      TypeId                type;     //! type of ring
      AreaAttributes        attributes;
      uint8_t               ring;
      std::vector<Id>       ids;
      std::vector<GeoCoord> nodes;

    public:
      inline Ring()
      : type(typeIgnore),
        ring(0)
      {
        // no code
      }

      inline void SetType(const TypeId& type)
      {
        this->type=type;
      }

      inline const AreaAttributes& GetAttributes() const
      {
        return attributes;
      }

      inline TypeId GetType() const
      {
        return type;
      }

      inline uint16_t GetFlags() const
      {
        return attributes.GetFlags();
      }

      inline std::string GetName() const
      {
        return attributes.GetName();
      }

      bool GetCenter(double& lat,
                     double& lon) const;

      void GetBoundingBox(double& minLon,
                          double& maxLon,
                          double& minLat,
                          double& maxLat) const;
    };

  private:
    FileOffset        fileOffset;

  public:
    std::vector<Ring> rings;

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

    inline TypeId GetType() const
    {
      return rings.front().GetType();
    }

    inline bool IsSimple() const
    {
      return rings.size()==1;
    }

    bool GetCenter(double& lat,
                   double& lon) const;
    void GetBoundingBox(double& minLon,
                        double& maxLon,
                        double& minLat,
                        double& maxLat) const;

    bool ReadIds(FileScanner& scanner,
                 uint32_t nodesCount,
                 std::vector<Id>& ids);
    bool ReadCoords(FileScanner& scanner,
                    uint32_t nodesCount,
                    std::vector<GeoCoord>& coords);
    bool Read(FileScanner& scanner);
    bool ReadOptimized(FileScanner& scanner);

    bool WriteIds(FileWriter& writer,
                  const std::vector<Id>& ids) const;
    bool WriteCoords(FileWriter& writer,
                    const std::vector<GeoCoord>& coords) const;
    bool Write(FileWriter& writer) const;
    bool WriteOptimized(FileWriter& writer) const;
  };

  typedef Ref<Area> AreaRef;
}

#endif
