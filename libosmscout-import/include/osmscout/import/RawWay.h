#ifndef OSMSCOUT_IMPORT_RAWWAY_H
#define OSMSCOUT_IMPORT_RAWWAY_H

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

#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class RawWay : public Referencable
  {
  private:
    OSMId              id;
    bool               isArea;
    std::vector<OSMId> nodes;
    FeatureValueBuffer featureValueBuffer;

  public:

  public:
    inline RawWay()
    : id(0),
      isArea(false)
    {
      // no code
    }

    inline OSMId GetId() const
    {
      return id;
    }

    inline bool IsArea() const
    {
      return isArea;
    }

    inline TypeInfoRef GetType() const
    {
      return featureValueBuffer.GetType();
    }

    inline TypeId GetTypeId() const
    {
      return featureValueBuffer.GetType()->GetId();
    }

    inline const std::vector<OSMId>& GetNodes() const
    {
      return nodes;
    }

    inline size_t GetNodeCount() const
    {
      return nodes.size();
    }

    inline OSMId GetNodeId(size_t idx) const
    {
      return nodes[idx];
    }

    inline OSMId GetFirstNodeId() const
    {
      return nodes[0];
    }

    inline OSMId GetLastNodeId() const
    {
      return nodes[nodes.size()-1];
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

    inline const FeatureValueBuffer& GetFeatureValueBuffer() const
    {
      return featureValueBuffer;
    }

    bool IsOneway() const;

    void SetId(OSMId id);
    void SetType(const TypeInfoRef& type,
                 bool area);
    void SetNodes(const std::vector<OSMId>& nodes);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const std::map<TagId,std::string>& tags);
    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    bool Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  typedef Ref<RawWay> RawWayRef;
}

#endif
