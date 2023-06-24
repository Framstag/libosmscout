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

#include <memory>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/util/Progress.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OSMSCOUT_API Node CLASS_FINAL
  {
  private:
    FeatureValueBuffer featureValueBuffer; //!< List of features

    FileOffset         fileOffset=0;         //!< File offset in the data file, use as unique id
    FileOffset         nextFileOffset=0;     //!< Offset after this node

    GeoCoord           coords;             //!< Coordinates of node

  public:
    Node() = default;

    FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    FileOffset GetNextFileOffset() const
    {
      return nextFileOffset;
    }

    ObjectFileRef GetObjectFileRef() const
    {
      return ObjectFileRef(fileOffset,refNode);
    }

    TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    const GeoCoord& GetCoords() const
    {
      return coords;
    }

    /**
     * Returns true if the nodes is in the given bounding box
     *
     * @param boundingBox
     *    bounding box to test for intersection
     * @return
     *    true on intersection, else false
     */
    bool Intersects(const GeoBox& boundingBox) const
    {
      return boundingBox.Includes(coords);
    }

    size_t GetFeatureCount() const
    {
      return featureValueBuffer.GetType()->GetFeatureCount();
    }

    bool HasFeature(size_t idx) const
    {
      return featureValueBuffer.HasFeature(idx);
    }

    const FeatureInstance& GetFeature(size_t idx) const
    {
      return featureValueBuffer.GetType()->GetFeature(idx);
    }

    void UnsetFeature(size_t idx)
    {
      featureValueBuffer.FreeValue(idx);
    }

    const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return featureValueBuffer;
    }

    void SetType(const TypeInfoRef& type);
    void SetCoords(const GeoCoord& coords);
    void SetFeatures(const FeatureValueBuffer& buffer);

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  using NodeRef = std::shared_ptr<Node>;
}

#endif
