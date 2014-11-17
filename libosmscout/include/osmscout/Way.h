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

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API Way : public Referencable
  {
  private:
    FeatureValueBuffer    featureValueBuffer; //! List of features

    FileOffset            fileOffset;


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
      return featureValueBuffer.HasValue(idx);
    }

    inline FeatureInstance GetFeature(size_t idx) const
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

    inline bool IsCircular() const
    {
      return (ids[0]!=0 && ids[0]==ids[ids.size()-1]);
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

    inline void SetType(const TypeInfoRef& type)
    {
      featureValueBuffer.SetType(type);
    }

    inline void SetFeatures(const FeatureValueBuffer& buffer)
    {
      featureValueBuffer.Set(buffer);
    }

    void SetLayerToMax();

    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    bool ReadOptimized(const TypeConfig& typeConfig,
                       FileScanner& scanner);

    bool Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
    bool WriteOptimized(const TypeConfig& typeConfig,
                        FileWriter& writer) const;
  };

  typedef Ref<Way> WayRef;
}

#endif
