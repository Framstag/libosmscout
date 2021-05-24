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

#include <memory>

#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {
  /**
   * Representation of an (complex/multipolygon) area.
   *
   * It consists from hierarchy of rings:
   *
   *  - optional master ring (ring number 0)
   *  - outer ring(s) (odd ring number)
   *    -- top level outer rings (ring number == 1)
   *    -- nested outer rings (odd ring number >= 3)
   *  - inner ring(s) (even ring number)
   *
   * This hierarchy may be presented as a tree, where ring number represent depth.
   * Master ring is first and the rest of rings are ordered deep-first fashion in Area::rings vector
   * (top-level outer rings are on the top of the tree).
   *
   * When object consists just from single outline (simple building for example),
   * Area contains just one (top level) outer ring
   *
   * When area is multipolygon relation (in OSM words), type and features of such relation
   * are stored in master ring. Every outer ring may have its own type and features.
   *
   * When outer ring has not type (GetType()->GetIgnore()), type of relation (master ring) should be used.
   * But OSM documentation is not clear what type should be used when outer ring has different type than relation.
   * For example this relation: https://www.openstreetmap.org/relation/7826515
   *  - master ring has type "leisure_park" and outer ring "place_islet".
   * OSM Scout library don't support multiple types for object, so in such cases,
   * we are using relation type for top-level outer ring and ring type for nested outer rings.
   *
   * When inner ring has no type (GetType()->GetIgnore())
   * it is used as simple clipping of containing (upper) outer ring.
   *
   * For example this ruin: https://www.openstreetmap.org/relation/7281899
   * Will have seven rings:
   *  [0] master (number 0) with type "building", without nodes
   *  [1] outer (number 1) without type, OSM id 295845013
   *  [2] inner (number 2) without type, OSM id 495919001
   *  [3] outer (number 3) without type, OSM id 495919003
   *  [4] outer (number 3) without type, OSM id 495919002
   *  [5] inner (number 2) without type, OSM id 495919008
   *  [6] inner (number 2) without type, OSM id 495919006
   *
   * Nested relations (member type=relation, role=inner|outer) are not supported for areas now.
   * See RelAreaDataGenerator::ResolveMultipolygonMembers code for the details.
   * For example relation https://www.openstreetmap.org/relation/7751062 will be created
   * just with master and one outer ring. Its nested relation (inner role)
   * https://www.openstreetmap.org/relation/7074095 will be imported as a separate Area.
   */
  class OSMSCOUT_API Area CLASS_FINAL
  {
  public:
    static const uint8_t masterRingId;
    static const uint8_t outerRingId;

  public:
    class OSMSCOUT_API Ring
    {
    private:
      FeatureValueBuffer    featureValueBuffer; //!< List of features
      uint8_t               ring=0;             //!< The ring hierarchy number (0...n)

    public:
      /**
       * Note that ring nodes, bbox and segments fields are public for simple manipulation.
       * User that modify it is responsible to keep these values in sync!
       * You should not rely on segments and bbox, it is just a cache used some algorithms.
       * It may be empty/invalid!
       */
      std::vector<Point>          nodes;        //!< The array of coordinates
      std::vector<SegmentGeoBox>  segments;     //!< Precomputed (cache) segment bounding boxes for optimisation
      GeoBox                      bbox;         //!< Precomputed (cache) bounding box

    public:
      Ring() = default;

      TypeInfoRef GetType() const
      {
        return featureValueBuffer.GetType();
      }

      size_t GetFeatureCount() const
      {
        return featureValueBuffer.GetType()->GetFeatureCount();
      }

      bool HasFeature(size_t idx) const
      {
        return featureValueBuffer.HasFeature(idx);
      }

      bool HasAnyFeaturesSet() const;

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

      bool IsMaster() const
      {
        return ring==masterRingId;
      }

      // top level outer ring
      bool IsTopOuter() const
      {
        return ring==outerRingId;
      }

      // ring level is odd, it is some outer ring
      bool IsOuter() const
      {
        return (ring & outerRingId) == outerRingId;
      }

      uint8_t GetRing() const
      {
        return ring;
      }

      Id GetSerial(size_t index) const
      {
        return nodes[index].GetSerial();
      }

      Id GetId(size_t index) const
      {
        return nodes[index].GetId();
      }

      Id GetFrontId() const
      {
        return nodes.front().GetId();
      }

      Id GetBackId() const
      {
        return nodes.back().GetId();
      }

      bool GetNodeIndexByNodeId(Id id,
                                size_t& index) const;

      const GeoCoord& GetCoord(size_t index) const
      {
        return nodes[index].GetCoord();
      }

      bool GetCenter(GeoCoord& center) const;

      void GetBoundingBox(GeoBox& boundingBox) const;
      GeoBox GetBoundingBox() const;

      void SetType(const TypeInfoRef& type)
      {
        featureValueBuffer.SetType(type);
      }

      void SetFeatures(const FeatureValueBuffer& buffer)
      {
        featureValueBuffer.Set(buffer);
      }

      void CopyMissingValues(const FeatureValueBuffer& buffer)
      {
        featureValueBuffer.CopyMissingValues(buffer);
      }

      void MarkAsMasterRing()
      {
        ring=masterRingId;
      }

      void MarkAsOuterRing()
      {
        ring=outerRingId;
      }

      void SetRing(uint8_t ring)
      {
        this->ring=ring;
      }

      friend class Area;
    };

    using RingVisitor = std::function<bool(size_t i, const Ring&, const TypeInfoRef&)>;

  private:
    FileOffset        fileOffset=0;
    FileOffset        nextFileOffset=0;

  public:
    std::vector<Ring> rings;

  public:
    Area() = default;

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
      return {fileOffset,refArea};
    }

    TypeInfoRef GetType() const
    {
      return rings.front().GetType();
    }

    TypeInfoRef GetRingType(const Ring &ring) const
    {
      if (ring.IsTopOuter() ||
          (ring.IsOuter() && ring.GetType()->GetIgnore())) {
        return GetType();
      }

      return ring.GetType();

    }

    const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return rings.front().GetFeatureValueBuffer();
    }

    bool IsSimple() const
    {
      return rings.size()==1;
    }

    bool GetCenter(GeoCoord& center) const;

    GeoBox GetBoundingBox() const;

    /**
     * Returns true if the bounding box of the object intersects the given
     * bounding box
     *
     * @param boundingBox
     *    bounding box to test for intersection
     * @return
     *    true on intersection, else false
     */
    bool Intersects(const GeoBox& boundingBox) const
    {
      return GetBoundingBox().Intersects(boundingBox);
    }

    /**
     * Read the area as written by Write().
     */
    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);

    /**
     * Read the area as written by WriteImport().
     */
    void ReadImport(const TypeConfig& typeConfig,
                    FileScanner& scanner);

    /**
     * Read the area as stored by WriteOptimized().
     */
    void ReadOptimized(const TypeConfig& typeConfig,
                       FileScanner& scanner);

    /**
     * Write the area with all data required in the
     * standard database.
     */
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;

    /**
     * Write the area with all data required during import,
     * certain optimizations done on the final data
     * are not done here to not loose information.
     */
    void WriteImport(const TypeConfig& typeConfig,
                     FileWriter& writer) const;

    /**
     * Write the area with all data required by the OptimizeLowZoom
     * index, dropping all ids.
     */
    void WriteOptimized(const TypeConfig& typeConfig,
                        FileWriter& writer) const;

    /**
     * Visit rings in breadth-first manner.
     * When visitor return true for some ring,
     * algorithm will continue deeper in hierarchy.
     */
    void VisitRings(const RingVisitor& visitor) const;

    /**
     * Visit possible clippings of ring specified by index.
     * We only take into account rings of the next level.
     */
    void VisitClippingRings(size_t index, const RingVisitor& visitor) const;
  };

  using AreaRef = std::shared_ptr<Area>;
}

#endif
