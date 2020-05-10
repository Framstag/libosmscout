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

#include <memory>

#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Progress.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OSMSCOUT_API Way CLASS_FINAL
  {
  private:
    FeatureValueBuffer featureValueBuffer; //!< List of features

    FileOffset         fileOffset=0;         //!< Offset into the data file of this way
    FileOffset         nextFileOffset=0;     //!< Offset after this way


  public:
    /**
     * Note that way nodes, bbox and segments fields are public for simple manipulation.
     * User that modify it is responsible to keep these values in sync!
     * You should not rely on segments and bbox, it is just a cache used some algorithms.
     * It may be empty/invalid!
     */
    std::vector<Point>          nodes;        //!< List of nodes
    std::vector<SegmentGeoBox>  segments;     //!< Precomputed (cache) segment bounding boxes for optimisation
    GeoBox                      bbox;         //!< Precomputed (cache) bounding box

  public:
    Way() = default;

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
      return {fileOffset,refWay};
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

    inline bool IsCircular() const
    {
      return nodes[0].GetId()!=0 &&
             nodes[0].GetId()==nodes[nodes.size()-1].GetId();
    }

    inline Id GetSerial(size_t index) const
    {
      return nodes[index].GetSerial();
    }

    inline Id GetId(size_t index) const
    {
      return nodes[index].GetId();
    }

    inline Id GetFrontId() const
    {
      return nodes.front().GetId();
    }

    inline Id GetBackId() const
    {
      return nodes.back().GetId();
    }

    inline const Point& GetPoint(size_t index) const
    {
      return nodes[index];
    }

    inline const GeoCoord& GetCoord(size_t index) const
    {
      return nodes[index].GetCoord();
    }

    inline GeoBox GetBoundingBox() const
    {
      if (bbox.IsValid() || nodes.empty()) {
        return bbox;
      }
      GeoBox boundingBox;

      osmscout::GetBoundingBox(nodes,
                               boundingBox);

      return boundingBox;
    }

    /**
     * Returns true if the bounding box of the object intersects the given
     * bounding box
     *
     * @param boundingBox
     *    bounding box to test for intersection
     * @return
     *    true on intersection, else false
     */
    inline bool Intersects(const GeoBox& boundingBox) const
    {
      return GetBoundingBox().Intersects(boundingBox);
    }

    bool GetCenter(GeoCoord& center) const;

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

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void ReadOptimized(const TypeConfig& typeConfig,
                       FileScanner& scanner);

    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
    void WriteOptimized(const TypeConfig& typeConfig,
                        FileWriter& writer) const;
  };

  using WayRef = std::shared_ptr<Way>;
}

#endif
