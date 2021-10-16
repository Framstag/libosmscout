#ifndef OSMSCOUT_IMPORT_PREPROCESS_H
#define OSMSCOUT_IMPORT_PREPROCESS_H

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

#include <thread>
#include <memory>
#include <unordered_map>

#include <osmscout/Tag.h>
#include <osmscout/routing/TurnRestriction.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/WorkQueue.h>

#include <osmscoutimport/Import.h>
#include <osmscoutimport/Preprocessor.h>
#include <osmscoutimport/RawCoastline.h>
#include <osmscoutimport/RawCoord.h>
#include <osmscoutimport/RawNode.h>
#include <osmscoutimport/RawWay.h>
#include <osmscoutimport/RawRelation.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {
  class Preprocess CLASS_FINAL : public ImportModule
  {
  public:
    static const char* const RAWCOORDS_DAT;
    static const char* const RAWNODES_DAT;
    static const char* const RAWWAYS_DAT;
    static const char* const RAWRELS_DAT;
    static const char* const RAWCOASTLINE_DAT;
    static const char* const RAWDATAPOLYGON_DAT;
    static const char* const RAWTURNRESTR_DAT;
    static const char* const RAWROUTEMASTER_DAT;
    static const char* const RAWROUTE_DAT;

  private:
    class Callback : public PreprocessorCallback
    {
    private:
      struct ProcessedData
      {
        std::vector<RawCoord>        rawCoords;
        std::vector<RawNode>         rawNodes;
        std::vector<RawWay>          rawWays;
        std::vector<RawCoastline>    rawCoastlines;
        std::vector<RawCoastline>    rawDatapolygon;
        std::vector<RawRelation>     multipolygons;
        std::vector<TurnRestriction> turnRestriction;
        std::vector<RawRelation>     routeMasters;
        std::vector<RawRelation>     routes;
      };

      // Should be unique_ptr but I get compiler errors if passing it to the WriteWorkerQueue
      typedef std::shared_ptr<ProcessedData> ProcessedDataRef;

    private:
      const TypeConfigRef                      typeConfig;
      const ImportParameter&                   parameter;
      Progress&                                progress;

      WorkQueue<ProcessedDataRef>              blockWorkerQueue;
      std::vector<std::thread>                 blockWorkerThreads;
      WorkQueue<void>                          writeWorkerQueue;
      std::thread                              writeWorkerThread;

      FileWriter                               rawCoordWriter;
      FileWriter                               nodeWriter;
      FileWriter                               wayWriter;
      FileWriter                               coastlineWriter;
      FileWriter                               datapolygonWriter;
      FileWriter                               turnRestrictionWriter;
      FileWriter                               multipolygonWriter;
      FileWriter                               routeMasterWriter;
      FileWriter                               routeWriter;

      bool                                     readNodes;
      bool                                     readWays;
      bool                                     readRelations;

      uint32_t                                 coordCount;
      uint32_t                                 nodeCount;
      uint32_t                                 wayCount;
      uint32_t                                 areaCount;
      uint32_t                                 relationCount;
      uint32_t                                 coastlineCount;
      uint32_t                                 datapolygonCount;
      uint32_t                                 turnRestrictionCount;
      uint32_t                                 multipolygonCount;
      uint32_t                                 routeMasterCount;
      uint32_t                                 routeCount;

      OSMId                                    lastNodeId;
      OSMId                                    lastWayId;
      OSMId                                    lastRelationId;

      OSMId                                    minNodeId;
      OSMId                                    maxNodeId;
      OSMId                                    minWayId;
      OSMId                                    maxWayId;
      OSMId                                    minRelationId;
      OSMId                                    maxRelationId;

      bool                                     nodeSortingError;
      bool                                     waySortingError;
      bool                                     relationSortingError;

      GeoCoord                                 minCoord;
      GeoCoord                                 maxCoord;

      std::vector<uint32_t>                    nodeStat;
      std::vector<uint32_t>                    areaStat;
      std::vector<uint32_t> wayStat;
      std::vector<uint32_t> relStat;

    private:
      bool IsTurnRestriction(const TagMap& tags,
                             TurnRestriction::Type& type) const;


      bool IsMultipolygon(const TagMap& tags,
                          TypeInfoRef& type);

      bool IsRouteMaster(const TagMap& tags,
                         TypeInfoRef& type);

      bool IsRoute(const TagMap& tags,
                   TypeInfoRef& type);

      bool DumpDistribution();
      bool DumpBoundingBox();

      void NodeSubTask(const RawNodeData& data,
                       ProcessedData& processed);
      void WaySubTask(const RawWayData& data,
                      ProcessedData& processed);
      void TurnRestrictionSubTask(const std::vector<RawRelation::Member>& members,
                                  TurnRestriction::Type type,
                                  ProcessedData& processed);
      void MultipolygonSubTask(const TagMap& tags,
                               const std::vector<RawRelation::Member>& members,
                               OSMId id,
                               const TypeInfoRef& type,
                               ProcessedData& processed);
      void RouteMasterSubTask(const TagMap& tags,
                              const std::vector<RawRelation::Member>& members,
                              OSMId id,
                              const TypeInfoRef& type,
                              ProcessedData& processed);
      void RouteSubTask(const TagMap& tags,
                        const std::vector<RawRelation::Member>& members,
                        OSMId id,
                        const TypeInfoRef& type,
                        ProcessedData& processed);
      void RelationSubTask(const RawRelationData& data,
                           ProcessedData& processed);

      ProcessedDataRef BlockTask(const RawBlockDataRef& data);
      void BlockWorkerLoop();

      void WriteTask(std::shared_future<ProcessedDataRef>& processed);
      void WriteWorkerLoop();

    public:
      Callback(const TypeConfigRef& typeConfig,
               const ImportParameter& parameter,
               Progress& progress);

      bool Initialize();

      void ProcessBlock(RawBlockDataRef data) override;

      bool Cleanup(bool success);
    };

  private:
    bool ProcessFiles(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress,
                      Callback& callback);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
