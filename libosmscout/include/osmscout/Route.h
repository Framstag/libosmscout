#ifndef OSMSCOUT_ROUTE_OBJ_H
#define OSMSCOUT_ROUTE_OBJ_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukáš Karas

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
#include <mutex>

#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Representation of route.
   *
   * Route is sequence of ways. Way may be part of multiple routes.
   * It may represent hiking, cycling, public transport (...) route.
   *
   * It is created from OpenStreetMap relation of "route" type,
   * as described on wiki: https://wiki.openstreetmap.org/wiki/Relation:route
   */
  class OSMSCOUT_API Route CLASS_FINAL
  {
  public:
    enum class MemberDirection {
      forward,
      backward
    };

    struct SegmentMember {
      MemberDirection direction{MemberDirection::forward};
      FileOffset way;
    };

    struct Segment {
      std::vector<SegmentMember> members;
    };

    using MemberCache=std::unordered_map<FileOffset,WayRef> ;

    std::vector<Segment> segments;
    GeoBox bbox;

  private:
    mutable std::mutex cacheMutex;
    MemberCache resolvedMembers; //!< Cache of resolved members used by some algorithms

    FeatureValueBuffer featureValueBuffer;   //!< List of features

    FileOffset         fileOffset=0;         //!< Offset into the data file of this way
    FileOffset         nextFileOffset=0;     //!< Offset after this way

  public:
    Route() = default;

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline FileOffset GetNextFileOffset() const
    {
      return nextFileOffset;
    }

    std::vector<FileOffset> GetMemberOffsets() const;

    bool HasResolvedMembers() const
    {
      std::lock_guard<std::mutex> lock(cacheMutex);
      return !resolvedMembers.empty();
    }

    void SetResolvedMembers(const MemberCache &map)
    {
      std::lock_guard<std::mutex> lock(cacheMutex);
      this->resolvedMembers=map;
    }

    MemberCache GetResolvedMembers() const
    {
      std::lock_guard<std::mutex> lock(cacheMutex);
      return resolvedMembers;
    }
    
    // inline ObjectFileRef GetObjectFileRef() const
    // {
    //   return {fileOffset,refRoute};
    // }

    inline GeoBox GetBoundingBox() const
    {
      return bbox;
    }

    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
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

    inline void UnsetFeature(size_t idx)
    {
      featureValueBuffer.FreeValue(idx);
    }

    inline const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return featureValueBuffer;
    }

    inline void SetType(const TypeInfoRef& type)
    {
      featureValueBuffer.SetType(type);
    }

    inline void SetFeatures(const FeatureValueBuffer& buffer)
    {
      featureValueBuffer.Set(buffer);
    }

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);

    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  using RouteRef = std::shared_ptr<Route>;
}

#endif

