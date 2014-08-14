#ifndef OSMSCOUT_NODE_H
#define OSMSCOUT_NODE_H

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

#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API Node : public Referencable
  {
  private:
    FileOffset         fileOffset;         //! File offset in the data file, use as unique id

    GeoCoord           coords;             //! Coordinates of node
    FeatureValueBuffer featureValueBuffer; //! List of features

  public:
    inline Node()
    : fileOffset(0)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

  public:
    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    inline TypeId GetTypeId() const
    {
      return featureValueBuffer.GetTypeId();
    }

    inline const GeoCoord& GetCoords() const
    {
      return coords;
    }

    inline double GetLat() const
    {
      return coords.GetLat();
    }

    inline double GetLon() const
    {
      return coords.GetLon();
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

    void SetType(const TypeInfoRef& type);
    void SetCoords(const GeoCoord& coords);
    void SetFeatures(const FeatureValueBuffer& buffer);

    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    bool Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef Ref<Node> NodeRef;
}

#endif
