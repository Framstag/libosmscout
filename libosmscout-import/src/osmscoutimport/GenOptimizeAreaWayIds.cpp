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

#include <osmscoutimport/GenOptimizeAreaWayIds.h>

#include <osmscoutimport/private/AreaWayIdReferenceRule.h>
#include <osmscoutimport/private/Config.h>

#if defined(HAVE_STD_EXECUTION)
#include <execution>
#endif

#include <algorithm>
#include <numeric>

#include <osmscout/io/DataFile.h>
#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/util/PolygonCenter.h>
#include <osmscout/util/StopClock.h>

#include <osmscoutimport/GenMergeAreas.h>
#include <osmscoutimport/GenWayWayDat.h>

namespace osmscout {

  const char* const OptimizeAreaWayIdsGenerator::AREAS3_TMP = "areas3.tmp";
  const char* const OptimizeAreaWayIdsGenerator::WAYS_TMP = "ways.tmp";

  class Processor
  {
  public:
    virtual ~Processor() = default;

    virtual bool operator()() = 0;
  };

  using ProcessorRef = std::shared_ptr<Processor>;

  class CopyAreasProcessor : public Processor
  {
  private:
    const TypeConfig             & typeConfig;
    const ImportParameter        & parameter;
    Progress                     & progress;
    const AreaWayIdReferenceRule & rule;

  public:
    CopyAreasProcessor(const TypeConfig& typeConfig,
                       const ImportParameter& parameter,
                       Progress& progress,
                       const AreaWayIdReferenceRule& rule)
    : typeConfig(typeConfig),
      parameter(parameter),
      progress(progress),
      rule(rule)
    {
    }

    std::optional<GeoCoord> OptionalRingCenter(const Area::Ring &ring)
    {
      // TODO: compute center just for types where we need
      // (types with Name feature, where it is expected that will be used for label)
      if (ring.nodes.empty()) {
        return std::nullopt;
      }
      auto bbox=ring.GetBoundingBox();
      double dimension=std::max(bbox.GetWidth(), bbox.GetHeight());
      auto center=PolygonCenter(ring.nodes,
                                dimension * 0.01);
      // when computed center is close to bbox center (20% of longest bbox side),
      // it is not necessary to store center coordinates to Ring
      auto bboxCenter=bbox.GetCenter();
      auto a=bboxCenter.GetLat()-center.GetLat();
      auto b=bboxCenter.GetLon()-center.GetLon();
      if (sqrt(a*a + b*b) < dimension * 0.2) {
        return std::nullopt;
      }
      return center;
    }

    bool operator()() override
    {
      FileScanner scanner;
      FileWriter  writer;

      try {
        uint32_t idClearedCount=0;
        StopClock phaseClock;

        progress.SetAction("Copy data from 'areas2.tmp' to 'areas3.tmp'");

        scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     MergeAreasGenerator::AREAS2_TMP),
                     FileScanner::Sequential,
                     parameter.GetAreaDataMemoryMaped());

        uint32_t areaCount=scanner.ReadUInt32();

        writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                            OptimizeAreaWayIdsGenerator::AREAS3_TMP));

        writer.Write(areaCount);

        for (uint32_t current=1; current<=areaCount; current++) {
          Area    data;

          progress.SetProgress(current,areaCount,MergeAreasGenerator::AREAS2_TMP);

          uint8_t type=scanner.ReadUInt8();
          OSMId   osmId=scanner.ReadInt64();

          data.ReadImport(typeConfig,
                          scanner);

          for (auto& ring : data.rings) {
            for (auto& node : ring.nodes) {
              if (!rule.KeepsSerial(node.GetId())) {
                node.ClearSerial();
                idClearedCount++;
              }
            }

            ring.center=OptionalRingCenter(ring);
          }

          writer.Write(type);
          writer.Write(osmId);

          data.Write(typeConfig,
                     writer);
        }

        scanner.Close();
        writer.Close();

        progress.Info(std::to_string(idClearedCount)+" node serials cleared");
        phaseClock.Stop();
        progress.Info("Phase 'Copy area data' took "+phaseClock.ResultString()+" s");
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());

        scanner.CloseFailsafe();
        writer.CloseFailsafe();

        return false;
      }

      return true;
    }
  };

  class CopyWaysProcessor : public Processor
  {
  private:
    const TypeConfig             & typeConfig;
    const ImportParameter        & parameter;
    Progress                     & progress;
    const AreaWayIdReferenceRule & rule;

  public:
    CopyWaysProcessor(const TypeConfig& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress,
                      const AreaWayIdReferenceRule& rule)
    : typeConfig(typeConfig),
      parameter(parameter),
      progress(progress),
      rule(rule)
    {}

    bool operator()() override
    {
      FileScanner scanner;
      FileWriter  writer;

      progress.SetAction("Copy data from 'wayway.tmp' to 'ways.tmp'");

      try {
        uint32_t idClearedCount=0;
        StopClock phaseClock;

        scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     WayWayDataGenerator::WAYWAY_TMP),
                     FileScanner::Sequential,
                     parameter.GetWayDataMemoryMaped());

        uint32_t dataCount=scanner.ReadUInt32();

        writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    OptimizeAreaWayIdsGenerator::WAYS_TMP));

        writer.Write(dataCount);

        for (uint32_t current=1; current<=dataCount; current++) {
          Way     data;

          progress.SetProgress(current,dataCount,WayWayDataGenerator::WAYWAY_TMP);

          uint8_t type=scanner.ReadUInt8();
          OSMId   osmId=scanner.ReadInt64();

          data.Read(typeConfig,
                    scanner);

          for (auto& node : data.nodes) {
            if (!rule.KeepsSerial(node.GetId())) {
              node.ClearSerial();
              idClearedCount++;
            }
          }

          writer.Write(type);
          writer.Write(osmId);

          data.Write(typeConfig,
                     writer);
        }

        scanner.Close();
        writer.Close();

        progress.Info(std::to_string(idClearedCount)+" node serials cleared");
        phaseClock.Stop();
        progress.Info("Phase 'Copy way data' took "+phaseClock.ResultString()+" s");
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());

        scanner.CloseFailsafe();
        writer.CloseFailsafe();

        return false;
      }

      return true;
    }
  };

  void OptimizeAreaWayIdsGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                   ImportModuleDescription& description) const
  {
    description.SetName("OptimizeAreaWayIdsGenerator");
    description.SetDescription("Optimize ids for areas and ways");

    description.AddRequiredFile(MergeAreasGenerator::AREAS2_TMP);
    description.AddRequiredFile(WayWayDataGenerator::WAYWAY_TMP);

    description.AddProvidedTemporaryFile(AREAS3_TMP);
    description.AddProvidedTemporaryFile(WAYS_TMP);
  }

  bool OptimizeAreaWayIdsGenerator::ScanAreaIds(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeConfig& typeConfig,
                                                AreaWayIdReferenceRule& rule)
  {
    FileScanner scanner;
    StopClock   phaseClock;

    progress.SetAction("Scanning ids from 'areas2.tmp'");

    try {
      uint32_t    idCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   MergeAreasGenerator::AREAS2_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      uint32_t dataCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=dataCount; current++) {
        Area    data;

        progress.SetProgress(current,dataCount,MergeAreasGenerator::AREAS2_TMP);

        /*uint8_t type=*/scanner.ReadUInt8();
        /*OSMId   osmId=*/scanner.ReadInt64();

        data.ReadImport(typeConfig,
                        scanner);

        for (const auto& ring: data.rings) {
          // We only need unique ids for areas and ways that are relevant for routing
          if (!ring.GetType()->CanRoute()) {
            continue;
          }

          idCount+=static_cast<uint32_t>(rule.AddObject(ring.nodes));
        }
      }

      progress.Info(std::to_string(dataCount)+" areas, "+std::to_string(idCount)+" ids found");
      phaseClock.Stop();
      progress.Info("Phase 'Scan area ids' took "+phaseClock.ResultString()+" s");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::ScanWayIds(const ImportParameter& parameter,
                                               Progress& progress,
                                               const TypeConfig& typeConfig,
                                               AreaWayIdReferenceRule& rule)
  {
    FileScanner scanner;
    StopClock   phaseClock;

    progress.SetAction("Scanning ids from 'wayway.tmp'");

    try {
      uint32_t idCount=0;
      uint32_t circularWayCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayWayDataGenerator::WAYWAY_TMP),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t dataCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=dataCount; current++) {
        Way     data;

        progress.SetProgress(current,dataCount,WayWayDataGenerator::WAYWAY_TMP);

        /*uint8_t type=*/scanner.ReadUInt8();
        /*OSMId   osmId=*/scanner.ReadInt64();

        data.Read(typeConfig,
                  scanner);

        if (!data.GetType()->CanRoute()) {
          continue;
        }

        idCount+=static_cast<uint32_t>(rule.AddObject(data.nodes));

        // If we have a circular way, we "fake" a double usage,
        // to make sure, that the node id of the first node
        // is not dropped later on, and we cannot detect
        // circular ways anymore
        if (data.IsCircular()) {
          rule.ForceSerial(data.GetBackId());
          circularWayCount++;
        }
      }

      progress.Info(std::to_string(dataCount)+" ways, "+std::to_string(idCount)+" ids, "+std::to_string(circularWayCount)+" circular ways found");
      phaseClock.Stop();
      progress.Info("Phase 'Scan way ids' took "+phaseClock.ResultString()+" s");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::Import(const TypeConfigRef& typeConfig,
                                           const ImportParameter& parameter,
                                           Progress& progress)
  {
    progress.SetAction("Optimize ids for areas and ways");

    AreaWayIdReferenceRule rule;

    StopClock              scanAreasClock;

    if (!ScanAreaIds(parameter,
                     progress,
                     *typeConfig,
                     rule)) {
      return false;
    }

    scanAreasClock.Stop();

    // The first scan has observed how many ids the areas reference. Use the count
    // it reports as the reserve, so that the set the ways are folded into does not
    // grow up from empty a second time.
    rule.ReserveReferencedIds(rule.GetReferencedIdCount());

    StopClock scanWaysClock;

    if (!ScanWayIds(parameter,
                    progress,
                    *typeConfig,
                    rule)) {
      return false;
    }

    scanWaysClock.Stop();

    progress.Info("Found "+std::to_string(rule.GetReferencedIdCount())+" relevant nodes, "+
                  std::to_string(rule.GetUsedAtLeastTwiceCount())+" of it at least used twice");

    rule.DiscardReferencedIds();

    StopClock                 copiesClock;

    std::vector<ProcessorRef> processors{
      std::make_shared<CopyAreasProcessor>(*typeConfig,
                                           parameter,
                                           progress,
                                           rule),
      std::make_shared<CopyWaysProcessor>(*typeConfig,
                                          parameter,
                                          progress,
                                          rule)};

    std::vector<bool> successes(processors.size(),false);

#if defined(HAVE_STD_EXECUTION)
    std::transform(std::execution::par_unseq,
                   processors.begin(),
                   processors.end(),
                   successes.begin(),
                   [](const ProcessorRef& processor) {
                     return (*processor)();
                   });
#else
    std::transform(processors.begin(),
                   processors.end(),
                   successes.begin(),
                   [](const ProcessorRef& processor) {
                     return (*processor)();
                   });
#endif
    copiesClock.Stop();

    progress.Info("Optimize ids: scan areas "+scanAreasClock.ResultString()+" s, scan ways "+
                  scanWaysClock.ResultString()+" s, copy phases "+copiesClock.ResultString()+" s");

    return std::accumulate(successes.begin(),
                           successes.end(),
                           true,
                           std::logical_and<bool>());
  }
}

