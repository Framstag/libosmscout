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

#include <osmscout/system/Compiler.h>

namespace osmscout {
  /**
    Representation of an (complex/multipolygon) area
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
      uint8_t               ring;               //!< The ring hierarchy number (0...n)

    public:
      std::vector<Point>    nodes;              //!< The array of coordinates

    public:
      inline Ring()
      : ring(0)
      {
        // no code
      }

      inline TypeInfoRef GetType() const
      {
        return featureValueBuffer.GetType();
      }

      inline void ClearFeatureValues()
      {
        featureValueBuffer.ClearFeatureValues();
      }

      inline size_t GetFeatureCount() const
      {
        return featureValueBuffer.GetType()->GetFeatureCount();
      }

      inline bool HasFeature(size_t idx) const
      {
        return featureValueBuffer.HasFeature(idx);
      }

      bool HasAnyFeaturesSet() const;

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

      inline bool IsMasterRing() const
      {
        return ring==masterRingId;
      }

      inline bool IsOuterRing() const
      {
        return ring==outerRingId;
      }

      inline uint8_t GetRing() const
      {
        return ring;
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

      inline const GeoCoord& GetCoord(size_t index) const
      {
        return nodes[index].GetCoord();
      }

      bool GetCenter(GeoCoord& center) const;

      void GetBoundingBox(GeoBox& boundingBox) const;

      inline void SetType(const TypeInfoRef& type)
      {
        featureValueBuffer.SetType(type);
      }

      inline void SetFeatures(const FeatureValueBuffer& buffer)
      {
        featureValueBuffer.Set(buffer);
      }

      inline void CopyMissingValues(const FeatureValueBuffer& buffer)
      {
        featureValueBuffer.CopyMissingValues(buffer);
      }

      inline void MarkAsMasterRing()
      {
        ring=masterRingId;
      }

      inline void MarkAsOuterRing()
      {
        ring=outerRingId;
      }

      inline void SetRing(uint8_t ring)
      {
        this->ring=ring;
      }

      inline void SetSerial(size_t index, uint8_t serial)
      {
        nodes[index].SetSerial(serial);
      }

      friend class Area;
    };

  private:
    FileOffset        fileOffset;
    FileOffset        nextFileOffset;

  public:
    std::vector<Ring> rings;

  public:
    inline Area()
    : fileOffset(0),nextFileOffset(0)
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
      return ObjectFileRef(fileOffset,refArea);
    }

    inline TypeInfoRef GetType() const
    {
      return rings.front().GetType();
    }

    inline const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return rings.front().GetFeatureValueBuffer();
    }

    inline bool IsSimple() const
    {
      return rings.size()==1;
    }

    bool GetCenter(GeoCoord& center) const;

    void GetBoundingBox(GeoBox& boundingBox) const;

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
      GeoBox objectBoundingBox;

      GetBoundingBox(objectBoundingBox);

      return objectBoundingBox.Intersects(boundingBox);
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
  };

  typedef std::shared_ptr<Area> AreaRef;
}

#endif
