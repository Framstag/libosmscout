#ifndef OSMSCOUT_IMPORT_PREPROCESS_OLT_H
#define OSMSCOUT_IMPORT_PREPROCESS_OLT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/import/Preprocessor.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/private/TestImportExport.h>

#include <osmscout/RegionList.h>

#include <osmscout/util/Geometry.h>

namespace osmscout {
  namespace test {
    class OSMSCOUT_TEST_API PreprocessOLT CLASS_FINAL : public Preprocessor
    {
    private:
      PreprocessorCallback&    callback;
      TagId                    tagAdminLevel;
      TagId                    tagBoundary;
      TagId                    tagName;
      TagId                    tagPlace;
      TagId                    tagHighway;
      TagId                    tagPostalCode;
      TagId                    tagBuilding;
      TagId                    tagAddrCity;
      TagId                    tagAddrPostcode;
      TagId                    tagAddrStreet;
      TagId                    tagAddrHousenumber;
      OSMId                    nodeId;
      OSMId                    wayId;
      std::map<GeoCoord,OSMId> coordNodeIdMap;
      std::map<OSMId,GeoCoord> nodeIdCoordMap;


    private:
      OSMId RegisterAndGetRawNodeId(PreprocessorCallback::RawBlockDataRef data,
                                    const GeoCoord& coord);

      void GenerateRegion(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          PreprocessorCallback::RawBlockDataRef data,
                          const Region& region,
                          const GeoBox& box,
                          GeoBoxPartitioner::Direction direction);

      void GenerateWaysAndNodes(const TypeConfigRef& typeConfig,
                                const ImportParameter& parameter,
                                Progress& progress,
                                const RegionList& regions);

    public:
      explicit PreprocessOLT(PreprocessorCallback& callback);

      bool Import(const TypeConfigRef& typeConfig,
                  const ImportParameter& parameter,
                  Progress& progress,
                  const std::string& filename) override;
    };
  }
}

#endif
