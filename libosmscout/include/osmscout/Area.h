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
#include <osmscout/PointSequence.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Progress.h>

namespace osmscout {
  /**
    Representation of an (complex/multipolygon) area
    */
  class OSMSCOUT_API Area
  {
  public:
    static const uint8_t masterRingId;
    static const uint8_t outerRingId;

  public:
    class OSMSCOUT_API Ring : public PointSequenceContainer
    {
    private:
      FeatureValueBuffer    featureValueBuffer; //!< List of features
      uint8_t               ring;               //!< The ring hierarchy number (0...n)

    public:
      inline Ring()
      : PointSequenceContainer(), ring(0)
      {
        // no code
      }
      virtual inline ~Ring()
      {
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

      bool GetCenter(GeoCoord& center) const;

      inline void SetType(const TypeInfoRef& type)
      {
        featureValueBuffer.SetType(type);
      }

      inline void SetFeatures(const FeatureValueBuffer& buffer)
      {
        featureValueBuffer.Set(buffer);
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

      friend class Area;
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

    inline TypeInfoRef GetType() const
    {
      return rings.front().GetType();
    }

    inline bool IsSimple() const
    {
      return rings.size()==1;
    }

    bool GetCenter(GeoCoord& center) const;

    void GetBoundingBox(GeoBox& boundingBox) const;

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
