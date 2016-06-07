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
#include <osmscout/PointSequence.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Progress.h>

namespace osmscout {

  class OSMSCOUT_API Way : public PointSequenceContainer
  {
  private:
    FeatureValueBuffer featureValueBuffer; //!< List of features

    FileOffset         fileOffset;         //!< Offset into the data file fo this way


  public:
    inline Way()
    : PointSequenceContainer(), fileOffset(0)
    {
      // no code
    }
    
    virtual inline ~Way()
    {  
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
      return (*nodes)[0].GetId()!=0 &&
             (*nodes)[0].GetId()==(*nodes)[nodes->size()-1].GetId();
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

    void SetLayerToMax();

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void ReadOptimized(const TypeConfig& typeConfig,
                       FileScanner& scanner);

    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
    void WriteOptimized(const TypeConfig& typeConfig,
                        FileWriter& writer) const;
  };

  typedef std::shared_ptr<Way> WayRef;
}

#endif
