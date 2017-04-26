#ifndef OSMSCOUT_IMPORT_RAWNODE_H
#define OSMSCOUT_IMPORT_RAWNODE_H

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

#include <memory>
#include <unordered_map>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class RawNode CLASS_FINAL
  {
  private:
    OSMId              id;
    GeoCoord           coord;
    FeatureValueBuffer featureValueBuffer;

  private:
  public:
    RawNode();

    inline OSMId GetId() const
    {
      return id;
    }

    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    inline const GeoCoord& GetCoords() const
    {
      return coord;
    }

    inline double GetLat() const
    {
      return coord.GetLat();
    }

    inline double GetLon() const
    {
      return coord.GetLon();
    }

    inline size_t GetFeatureCount() const
    {
      return featureValueBuffer.GetType()->GetFeatureCount();
    }

    inline bool HasFeature(size_t idx) const
    {
      return featureValueBuffer.HasFeature(idx);
    }

    inline const FeatureInstance& GetFeature(size_t idx) const
    {
      return featureValueBuffer.GetType()->GetFeature(idx);
    }

    inline FeatureValue* GetFeatureValue(size_t idx) const
    {
      return featureValueBuffer.GetValue(idx);
    }

    inline const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return featureValueBuffer;
    }

    inline bool IsIdentical(const RawNode& other) const
    {
      return id==other.id;
    }

    inline bool IsSame(const RawNode& other) const
    {
      return coord==other.coord;
    }

    inline bool IsEqual(const RawNode& other) const
    {
      return id==other.id || coord==other.coord;
    }

    void SetId(OSMId id);
    void SetType(const TypeInfoRef& type);

    void SetCoord(const GeoCoord& coord);

    void UnsetFeature(size_t idx);

    void Parse(TagErrorReporter& errorReporter,
               const TypeConfig& typeConfig,
               const TagMap& tags);
    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef std::shared_ptr<RawNode> RawNodeRef;
}

#endif
