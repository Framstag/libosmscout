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

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OSMSCOUT_API Node CLASS_FINAL
  {
  private:
    FeatureValueBuffer featureValueBuffer; //!< List of features

    FileOffset         fileOffset;         //!< File offset in the data file, use as unique id
    FileOffset         nextFileOffset;     //!< Offset after this node

    GeoCoord           coords;             //!< Coordinates of node

  public:
    inline Node()
    : fileOffset(0), nextFileOffset(0)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline FileOffset GetNextFileOffset() const
    {
      return nextFileOffset;
    }

    inline ObjectFileRef GetObjectFileRef() const
    {
      return ObjectFileRef(fileOffset,refNode);
    }

    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    inline const GeoCoord& GetCoords() const
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
    inline bool Intersects(const GeoBox& boundingBox) const
    {
      return boundingBox.Includes(coords);
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

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef std::shared_ptr<Node> NodeRef;
}

#endif
