/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <osmscoutimport/GenMergeAreas.h>

#include <numeric>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/async/ProcessingQueue.h>
#include <osmscout/async/Worker.h>

#include <osmscoutimport/MergeAreaData.h>

namespace osmscout {

  const char* const MergeAreasGenerator::AREAS2_TMP="areas2.tmp";

  static bool HasNoDuplicateNodes(const std::vector<Point>& points)
  {
    std::set<GeoCoord> coordSet;

    for (const auto& point : points) {
      if (!coordSet.insert(point.GetCoord()).second) {
        return false;
      }
    }

    return true;
  }

  /**
   * Index areas by their "at least used twice" nodes
   * @param nodeUseMap
   *    Node use map
   * @param areas
   *    List of areas
   * @param idAreaMap
   *    Resulting index structure, mammping a node id to the
   *    list of areas that hold this node in on of their outer
   *    rings.
   */
  static void IndexAreasByNodeIds(const std::unordered_set<Id>& nodeUseMap,
                                  const std::list<AreaRef>& areas,
                                  std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (const auto& area : areas) {
      std::unordered_set<Id> nodeIds;

      for (const auto& ring: area->rings) {
        if (ring.IsTopOuter()) {
          for (const auto& node : ring.nodes) {
            Id id=node.GetId();

            if (nodeIds.find(id)==nodeIds.end() &&
                nodeUseMap.find(id)!=nodeUseMap.end()) {
              idAreaMap[id].insert(area);
              nodeIds.insert(id);
            }
          }
        }
      }
    }
  }

  static void EraseAreaInCache(const std::unordered_set<Id>& nodeUseMap,
                               const AreaRef& area,
                               std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (const auto& ring: area->rings) {
      if (ring.IsTopOuter()) {
        for (const auto& node : ring.nodes) {
          Id id=node.GetId();

          if (nodeUseMap.find(id)!=nodeUseMap.end()) {
            idAreaMap[id].erase(area);
          }
        }
      }
    }
  }

  /**
   * Return the index of the first outer ring that contains the given node id.
   */
  static size_t GetFirstOuterRingWithId(const Area& area,
                                        Id id)
  {
    for (size_t r=0; r<area.rings.size(); r++) {
      if (area.rings[r].IsTopOuter()) {
        for (const auto& node : area.rings[r].nodes) {
          if (node.GetId()==id) {
            return r;
          }
        }
      }
    }

    assert(false);

    return 0;
  }

  /**
 * @param nodeUseMap
 * @param area May be manipulated by merging with other areas
 * @param idAreaMap Map of all merge candidates by node id, necessary for finding merge candidates, merges will be removed
 * @param finishedIds Ids I have already merged for this area
 * @param mergedAway fileOffsets of areas that have been merged with other areas and thus obsolete
 * @return
 */
  static bool TryMerge(const std::unordered_set<Id>& nodeUseMap,
                       Area& area,
                       std::unordered_map<Id,std::set<AreaRef> >& idAreaMap,
                       std::set<Id>& finishedIds,
                       std::unordered_set<FileOffset>& mergedAway)
  {
    for (const auto& ring: area.rings) {
      if (!ring.IsTopOuter()) {
        continue;
      }

      std::unordered_set<Id> nodeIds;
      std::set<AreaRef>      visitedAreas; // It is possible that two areas intersect in multiple nodes, to avoid multiple merge tests, we monitor the visited areas

      for (const auto& node : ring.nodes) {
        Id id=node.GetId();

        if (finishedIds.find(id)!=finishedIds.end()) {
          continue;
        }

        // node already occurred
        if (nodeIds.find(id)!=nodeIds.end()) {
          continue;
        }

        // Track, that we visited this node
        nodeIds.insert(id);

        // Uninteresting node
        if (nodeUseMap.find(id)==nodeUseMap.end()) {
          continue;
        }

        // We now have an node id other areas with the same node id exist for.
        // Let's take a look at all these candidates

        auto candidate=idAreaMap[id].begin();

        while (candidate!=idAreaMap[id].end()) {
          AreaRef candidateArea(*candidate);

          // We only merge against "simple" areas
          if (candidateArea->rings.size()!=1) {
            ++candidate;
            continue;
          }

          // The area itself => skip
          if (area.GetFileOffset()==candidateArea->GetFileOffset()) {
            // TODO: Should not happen?
            ++candidate;
            continue;
          }

          // Already visited during this merge => skip
          if (visitedAreas.find(candidateArea)!=visitedAreas.end()) {
            ++candidate;
            continue;
          }

          // Area was already merged before => skip
          if (mergedAway.find(candidateArea->GetFileOffset())!=mergedAway.end()) {
            // TODO: Should not happen?
            ++candidate;
            continue;
          }

          size_t firstOuterRing=GetFirstOuterRingWithId(area,
                                                        id);

          size_t secondOuterRing=GetFirstOuterRingWithId(*candidateArea,
                                                         id);

          if (area.rings[firstOuterRing].nodes.size()+candidateArea->rings[secondOuterRing].nodes.size()>
              FileWriter::MAX_NODES) {
            // We could merge, but we could not store the resulting area anymore
            ++candidate;
            continue;
          }

          // Flag as visited (the areas might be registered for other
          // nodes shared by both areas, too) and we do not want to
          // analyze it again
          visitedAreas.insert(candidateArea);

          // Areas do not have the same feature value buffer values, skip
          if (area.rings[firstOuterRing].GetFeatureValueBuffer()!=
              candidateArea->rings[secondOuterRing].GetFeatureValueBuffer()) {
            //std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << " because of different feature values" << std::endl;
            ++candidate;
            continue;
          }

          // Yes, we can merge!

          PolygonMerger merger;

          merger.AddPolygon(area.rings[firstOuterRing].nodes);
          merger.AddPolygon(candidateArea->rings[secondOuterRing].nodes);

          std::list<PolygonMerger::Polygon> result;

          if (merger.Merge(result)) {
            if (result.size()==1 && HasNoDuplicateNodes(result.front().coords)) {
              //std::cout << "MERGE areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;

              area.rings[firstOuterRing].nodes=result.front().coords;

              EraseAreaInCache(nodeUseMap,
                               candidateArea,
                               idAreaMap);

              mergedAway.insert(candidateArea->GetFileOffset());

              return true;
            }
            //std::cout << "COULD merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
          }
          //std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << " at " << index << " " << node.GetCoord().GetDisplayText() << std::endl;

          ++candidate;
        }

        finishedIds.insert(id);
      }
    }

    return false;
  }

  /**
   * Merge area data of one type
   *
   * @param progress
   * @param job
   */
  static AreaMergeResult MergeAreas(Progress& progress,
                                    AreaMergeJob& job)
  {
    std::unordered_map<Id,std::set<AreaRef> > idAreaMap;
    size_t                                    overallCount=job.areas.size();
    AreaMergeResult                           mergeResult;

    mergeResult.type=job.type;

    IndexAreasByNodeIds(job.nodeUseSet,
                        job.areas,
                        idAreaMap);

    progress.Info("Found "+std::to_string(idAreaMap.size())+" nodes as possible connection points for areas");

    while (!job.areas.empty()) {
      AreaRef area;

      progress.SetProgress(overallCount-job.areas.size(),
                           overallCount,
                           job.type->GetName());

      // Pop a area from the list of "to be processed" areas
      area=job.areas.front();
      job.areas.pop_front();

      // This areas has already be "handled", ignore it
      if (mergeResult.mergedAway.find(area->GetFileOffset())!=mergeResult.mergedAway.end()) {
        continue;
      }

      // Delete all mentioning of this area in the idAreaMap cache
      EraseAreaInCache(job.nodeUseSet,
                       area,
                       idAreaMap);

      std::set<Id> finishedIds;

      // Now try to merge it against candidates that share the same ids.
      if (TryMerge(job.nodeUseSet,
                   *area,
                   idAreaMap,
                   finishedIds,
                   mergeResult.mergedAway)) {
        mergeResult.merges.push_back(area);

        while (TryMerge(job.nodeUseSet,
                        *area,
                        idAreaMap,
                        finishedIds,
                        mergeResult.mergedAway)) {
          // no code
        }
      }
    }

    return mergeResult;
  }

  class MergeWorker CLASS_FINAL : public Pipe<AreaMergeJob,AreaMergeResult>
  {
  private:
    Progress& progress;

  private:
    AreaMergeResult ProcessJob(AreaMergeJob& job)
    {
      progress.Info("Merging areas of type "+job.type->GetName());

      StopClock       mergeStopClock;

      AreaMergeResult result=MergeAreas(progress,
                                        job);

      mergeStopClock.Stop();
      progress.Info(
        "Reduced areas of '"+job.type->GetName()+"' from "+std::to_string(job.areaCount)+
        " to "+std::to_string(job.areaCount-result.mergedAway.size())+
        " in "+mergeStopClock.ResultString()+" second(s)");

      return result;
    }

    void ProcessingLoop() override
    {
      while (!inQueue.Finished()) {
        std::optional<AreaMergeJob> value=inQueue.PopTask();

        if (value) {
          AreaMergeJob job=std::move(value.value());

          AreaMergeResult result=ProcessJob(job);

          outQueue.PushTask(result);
        }
      }

      outQueue.Stop();
    }

  public:
    MergeWorker(Progress& progress,
                osmscout::ProcessingQueue<AreaMergeJob>& inQueue,
                osmscout::ProcessingQueue<AreaMergeResult>& outQueue)
      : Pipe(inQueue,
             outQueue),
        progress(progress)
    {
      Start();
    }
  };

  static std::vector<AreaMergeResult> MergeAreas(const TypeConfig& typeConfig,
                                                 const TypeInfoSet& loadedTypes,
                                                 Progress& progress,
                                                 std::vector<AreaMergeJob>& mergeJobs /* we move the job into the queue so no const */)
  {
    progress.SetAction("Merging areas with {} thread(s)",std::thread::hardware_concurrency());

    StopClock stopClock;

    std::vector<AreaMergeResult>              mergeResult(typeConfig.GetTypeCount());
    ProcessingQueue<AreaMergeJob>             queue1(10000);
    ProcessingQueue<AreaMergeResult>          queue2(10000);
    std::vector<AreaMergeJob>                 mergeJobList;
    ThreadedWorkerPool<MergeWorker>           workerPool(progress,
                                                         queue1,
                                                         queue2);

    for (const auto& type : loadedTypes) {
      if (!mergeJobs[type->GetIndex()].areas.empty()) {
        mergeJobList.push_back(std::move(mergeJobs[type->GetIndex()]));
      }
    }

    // Sort job list by area count (job with most areas first) to increase chance for usage of all threads
    std::ranges::sort(mergeJobList,
                      [](const AreaMergeJob& a,
                         const AreaMergeJob& b) {
                        return a.areas.size()>b.areas.size();
                      });

    // Push all jobs
    for (auto& job : mergeJobList) {
      queue1.PushTask(std::move(job));
    }
    queue1.Stop();

    workerPool.Wait();

    // Read worker results from queue until queue is empty
    while (true) {
      std::optional<AreaMergeResult> result=queue2.PopTask();

      if (!result) {
        break;
      }

      mergeResult[result.value().type->GetIndex()]=std::move(result.value());
    }

    stopClock.Stop();

    progress.SetAction("Merged {} types in {}",loadedTypes.Size(),stopClock.ResultString());

    return mergeResult;
  }

void MergeAreasGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                           ImportModuleDescription& description) const
  {
    description.SetName("MergeAreasGenerator");
    description.SetDescription("Merge areas into bigger areas");

    description.AddRequiredFile(MergeAreaDataGenerator::AREAS_TMP);

    description.AddProvidedTemporaryFile(AREAS2_TMP);
  }

  /**
   * Scan all areas for node ids that occur in more than one area. Only areas with
   * nodes that are used in other areas, too, are candidates for merging.
   *
   * We only take nodes in the outer rings into account.
   *
   * @param progress
   *   Progress
   * @param typeConfig
   *   TypeConfiguration
   * @param scanner
   *    FileScanner for area data
   * @param writer
   *    FileWriter for merged area data
   * @param mergeTypes
   *   Set of types for which we do area merging
   * @param areasWritten
   *    Number of areas that have already been written to the writer,
   *    because the have a area type that is not merged
   */
  static std::vector<AreaMergeJob> ScanAreaNodeIds(Progress& progress,
                                                   const TypeConfig& typeConfig,
                                                   FileScanner& scanner,
                                                   FileWriter& writer,
                                                   const TypeInfoSet& mergeTypes,
                                                   uint32_t& areasWritten)
  {
    progress.SetAction("Scanning for nodes joining areas from '{}'",scanner.GetFilename());

    std::vector<AreaMergeJob> mergeJobs(typeConfig.GetTypeCount());

    for (const auto& type : typeConfig.GetTypes()) {
      mergeJobs[type->GetIndex()].type=type;
    }

    StopClock stopClock;

    scanner.GotoBegin();

    uint32_t areaCount=scanner.ReadUInt32();

    Area area;

    std::unordered_set<Id> usedOnceSet;

    for (uint32_t current=1; current<=areaCount; current++) {
      progress.SetProgress(current,
                           areaCount);

      uint8_t type=scanner.ReadUInt8();
      Id      id=scanner.ReadUInt64();

      area.ReadImport(typeConfig,
                      scanner);

      if (!mergeTypes.IsSet(area.GetType())) {
        writer.Write(type);
        writer.Write(id);

        area.WriteImport(typeConfig,
                         writer);

        areasWritten++;

        continue;
      }

      // We insert every node id only once per area, because we want to
      // find nodes that are shared by *different* areas.

      // We currently only merge simple areas with one (outer) ring
      if (area.rings.size()==1) {
        std::unordered_set<Id> nodeIds;

        for (const auto& ring: area.rings) {
          if (!ring.IsTopOuter()) {
            continue;
          }

          for (const auto& node : ring.nodes) {
            nodeIds.insert(node.GetId());
          }
        }

        for (auto nodeId : nodeIds) {
          if (usedOnceSet.find(nodeId)!=usedOnceSet.end()) {
            mergeJobs[area.GetType()->GetIndex()].nodeUseSet.insert(nodeId);
          }
          else {
            usedOnceSet.insert(nodeId);
          }
        }
      }
    }

    stopClock.Stop();

    progress.Info("Scanning took "+stopClock.ResultString()+" second(s)");

    return mergeJobs;
  }

  /**
   * Load areas which have one of the types given by candidateTypes. If at least one node
   * in one of the outer rings of the areas is marked in nodeUseMap as "used at least twice",
   * add it to the mergeJob type array.
   *
   * If the number of indexed areas is bigger than parameter.GetRawWayBlockSize() mergeJobs for types are
   * dropped until the number is again below the limit.
   *
   * At he same type write read areas that are not of a mergable type to the destination file -
   * if we are called for the first time.
   */

  void MergeAreasGenerator::GetAreas(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     const TypeInfoSet& candidateTypes,
                                     TypeInfoSet& loadedTypes,
                                     FileScanner& scanner,
                                     std::vector<AreaMergeJob>& mergeJobs)
  {
    size_t collectedAreasCount=0;
    size_t typesWithAreas     =0;

    progress.SetAction("Collecting area data by type ({} type(s))",candidateTypes.Size());

    StopClock stopClock;

    for (auto& job : mergeJobs) {
      job.areaCount=0;
    }

    loadedTypes=candidateTypes;

    scanner.GotoBegin();

    uint32_t areaCount=scanner.ReadUInt32();

    for (uint32_t a=1; a<=areaCount; a++) {
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,
                           areaCount);

      /*uint8_t type=*/scanner.ReadUInt8();
      /*Id      id  =*/scanner.ReadUInt64();

      area->ReadImport(typeConfig,
                       scanner);

      mergeJobs[area->GetType()->GetIndex()].areaCount++;

      // This is an area of a type that does not get merged,
      // we directly store it in the target file.
      if (!loadedTypes.IsSet(area->GetType())) {
        continue;
      }

      bool isMergeCandidate=false;

      // We currently only merge simple areas with one (outer) ring
      if (area->rings.size()==1) {
        for (const auto& ring: area->rings) {
          if (!ring.IsTopOuter()) {
            continue;
          }

          for (const auto& node : ring.nodes) {
            const auto& nodeUseSet=mergeJobs[area->GetType()->GetIndex()].nodeUseSet;

            if (nodeUseSet.find(node.GetId())!=
                nodeUseSet.end()) {
              // contains at least one joinable node
              isMergeCandidate=true;
              break;
            }
          }

          if (isMergeCandidate) {
            break;
          }
        }
      }

      if (!isMergeCandidate) {
        continue;
      }

      if (mergeJobs[area->GetType()->GetIndex()].areas.empty()) {
        typesWithAreas++;
      }

      mergeJobs[area->GetType()->GetIndex()].areas.push_back(area);

      collectedAreasCount++;

      while (collectedAreasCount>parameter.GetRawWayBlockSize() &&
             typesWithAreas>1) {
        TypeInfoRef victimType;

        // Find the type with the smallest amount of ways loaded
        for (const auto& loadedType : loadedTypes) {
          if (!mergeJobs[loadedType->GetIndex()].areas.empty() &&
              (!victimType ||
               mergeJobs[loadedType->GetIndex()].areas.size()<mergeJobs[victimType->GetIndex()].areas.size())) {
            victimType=loadedType;
          }
        }

        // If there is more then one type of way, we always must find a "victim" type.
        assert(victimType);

        // Correct the statistics
        collectedAreasCount-=mergeJobs[victimType->GetIndex()].areas.size();
        // Clear already loaded data of th victim type
        mergeJobs[victimType->GetIndex()].areas.clear();

        typesWithAreas--;
        loadedTypes.Remove(victimType);
      }
    }

    stopClock.Stop();

    progress.SetAction(
      "Collected "+std::to_string(collectedAreasCount)+" areas for "+std::to_string(loadedTypes.Size())+" types in "+
      stopClock.ResultString()+" second(s)");
  }

  /**
   * Walk through the inout file and for every area type with merge=true store the
   * original area or the merge area while dropping the merged-away areas.
   *
   * @param progress
   * @param typeConfig
   * @param scanner
   * @param writer
   * @param loadedTypes
   * @param mergeJob
   * @param areasWritten
   */
  void MergeAreasGenerator::WriteMergeResult(Progress& progress,
                                             const TypeConfig& typeConfig,
                                             FileScanner& scanner,
                                             FileWriter& writer,
                                             const TypeInfoSet& loadedTypes,
                                             const std::vector<AreaMergeResult>& mergeJob,
                                             uint32_t& areasWritten)
  {
    std::unordered_map<FileOffset,AreaRef> merges;
    std::unordered_set<FileOffset>         ignores;

    for (const auto& type : loadedTypes) {
      for (const auto& area : mergeJob[type->GetIndex()].merges) {
        merges[area->GetFileOffset()]=area;
      }

      ignores.insert(mergeJob[type->GetIndex()].mergedAway.begin(),
                     mergeJob[type->GetIndex()].mergedAway.end());
    }

    scanner.GotoBegin();

    uint32_t areaCount=scanner.ReadUInt32();

    for (uint32_t a=1; a<=areaCount; a++) {
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,
                           areaCount);

      uint8_t type=scanner.ReadUInt8();
      Id      id  =scanner.ReadUInt64();

      area->ReadImport(typeConfig,
                       scanner);

      if (loadedTypes.IsSet(area->GetType())) {
        if (ignores.find(area->GetFileOffset())!=ignores.end()) {
          continue;
        }

        writer.Write(type);
        writer.Write(id);

        const auto& merge=merges.find(area->GetFileOffset());

        if (merge!=merges.end()) {
          area=merge->second;
        }

        area->WriteImport(typeConfig,
                          writer);

        areasWritten++;
      }
    }
  }

  /**
   * Tries to merge smaller areas into bigger areas if their types and attributes are the same
   *
   * Principal processing steps
   * * Determine area types to be merged
   * * Scan area file
   *   * For to-be-merged area types determine more-than-once used node ids
   *   * For not-to-be-merged area types write data to new output file
   * * Rescan area file (in principle for each area type)
   *   * Load all areas into memory that are merge candidates
   *   * Write all areas to file that are not merge candidates
   * * Recursively merge all merge candidates
   * * Write result of merges (merged on non-merged-candidates) to file
   * * Write number of file entries to file
   *
   * @param typeConfig
   *    Type configuration
   * @param parameter
   *    An import parameter
   * @param progress
   *    Progress reporting object
   * @return
   *    true on success, else false
   */
  bool MergeAreasGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    TypeInfoSet mergeTypes;
    FileScanner scanner;
    FileWriter  writer;

    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->GetMergeAreas()) {
        mergeTypes.Set(type);
      }
    }

    try {
      uint32_t areasWritten=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   MergeAreaDataGenerator::AREAS_TMP),
                   FileScanner::Sequential,
                   parameter.GetRawWayDataMemoryMaped());

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AREAS2_TMP));

      writer.Write(areasWritten);

      std::vector<AreaMergeJob> mergeJobs=ScanAreaNodeIds(progress,
                                                          *typeConfig,
                                                          scanner,
                                                          writer,
                                                          mergeTypes,
                                                          areasWritten);

      size_t nodeCount=std::accumulate(mergeJobs.begin(),
                                       mergeJobs.end(),
                                       0,
                                       [](size_t sum,
                                          const AreaMergeJob& job) {
                                         return sum+job.nodeUseSet.size();
                                       });

      progress.Info("Found "+std::to_string(nodeCount)+" nodes as possible connection points for areas");

      /* ------ */

      while (true) {
        TypeInfoSet loadedTypes;

        //
        // Load type data
        //

        GetAreas(parameter,
                 progress,
                 *typeConfig,
                 mergeTypes,
                 loadedTypes,
                 scanner,
                 mergeJobs);

        std::vector<AreaMergeResult> mergeResult=MergeAreas(*typeConfig,
                                                            loadedTypes,
                                                            progress,
                                                            mergeJobs);

        // Write merge results

        WriteMergeResult(progress,
                         *typeConfig,
                         scanner,
                         writer,
                         loadedTypes,
                         mergeResult,
                         areasWritten);

        mergeTypes.Remove(loadedTypes);

        if (mergeTypes.Empty()) {
          break;
        }
      }

      scanner.Close();

      writer.GotoBegin();
      writer.Write(areasWritten);
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
