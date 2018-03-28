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

#include <osmscout/import/GenMergeAreas.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/import/MergeAreaData.h>

namespace osmscout {

  const char* MergeAreasGenerator::AREAS2_TMP="areas2.tmp";

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

  void MergeAreasGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                           ImportModuleDescription& description) const
  {
    description.SetName("MergeAreasGenerator");
    description.SetDescription("Merge areas into bigger areas");

    description.AddRequiredFile(MergeAreaDataGenerator::AREAS_TMP);

    description.AddProvidedTemporaryFile(AREAS2_TMP);
  }

  /**
   * Returns the index of the first outer ring that contains the given id.
   */
  size_t MergeAreasGenerator::GetFirstOuterRingWithId(const Area& area,
                                                      Id id) const
  {
    for (size_t r = 0; r<area.rings.size(); r++) {
      if (area.rings[r].IsOuterRing()) {
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

  void MergeAreasGenerator::EraseAreaInCache(const std::unordered_set<Id>& nodeUseMap,
                                             const AreaRef& area,
                                             std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (const auto& ring: area->rings) {
      if (ring.IsOuterRing()) {
        for (const auto node : ring.nodes) {
          Id id=node.GetId();

          if (nodeUseMap.find(id)!=nodeUseMap.end()) {
            idAreaMap[id].erase(area);
          }
        }
      }
    }
  }

  /**
   * Scans all areas. If an areas is of one of the given merge types, index all node ids
   * into the given NodeUseMap for all outer rings of the given area.
   */
  bool MergeAreasGenerator::ScanAreaNodeIds(Progress& progress,
                                            const TypeConfig& typeConfig,
                                            FileScanner& scanner,
                                            const TypeInfoSet& mergeTypes,
                                            std::unordered_set<Id>& nodeUseMap)
  {
    uint32_t areaCount=0;

    progress.SetAction("Scanning for nodes joining areas from '"+scanner.GetFilename()+"'");

    scanner.GotoBegin();

    scanner.Read(areaCount);

    uint8_t type;
    Id      id;
    Area    data;

    std::unordered_set<Id> usedOnceSet;

    for (uint32_t current=1; current<=areaCount; current++) {
      progress.SetProgress(current,areaCount);

      scanner.Read(type);
      scanner.Read(id);

      data.ReadImport(typeConfig,
                      scanner);

      if (!mergeTypes.IsSet(data.GetType())) {
        continue;
      }

      // We insert every node id only once per area, because we want to
      // find nodes that are shared by *different* areas.

      std::unordered_set<Id> nodeIds;

      for (const auto& ring: data.rings) {
        if (!ring.IsOuterRing()) {
          continue;
        }

        for (const auto node : ring.nodes) {
          Id id=node.GetId();

          if (nodeIds.find(id)==nodeIds.end()) {
            auto entry=usedOnceSet.find(id);

            if (entry!=usedOnceSet.end()) {
              nodeUseMap.insert(id);
            }
            else {
              usedOnceSet.insert(id);
            }
            nodeIds.insert(id);
          }
        }
      }
    }

    return true;
  }

  /**
   * Load areas which has a one for the types given by types. If at leats one node
   * in one of the outer rings of the areas is marked in nodeUseMap as "used at least twice",
   * index it into the areas map.
   *
   * If the number of indexed areas is bigger than parameter.GetRawWayBlockSize() types are
   * dropped form areas until the number is again below the lmit.
   */
  bool MergeAreasGenerator::GetAreas(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     const TypeInfoSet& candidateTypes,
                                     TypeInfoSet& loadedTypes,
                                     const std::unordered_set<Id>& nodeUseMap,
                                     FileScanner& scanner,
                                     FileWriter& writer,
                                     std::vector<AreaMergeData>& mergeJob,
                                     uint32_t& areasWritten)
  {
    bool        firstCall=areasWritten==0; // We are called for the first time
    uint32_t    areaCount=0;
    size_t      collectedAreasCount=0;
    size_t      typesWithAreas=0;

    for (auto& data : mergeJob) {
      data.areaCount=0;
    }

    loadedTypes=candidateTypes;

    scanner.GotoBegin();

    scanner.Read(areaCount);

    for (uint32_t a=1; a<=areaCount; a++) {
      uint8_t type;
      Id      id;
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,areaCount);

      scanner.Read(type);
      scanner.Read(id);

      area->ReadImport(typeConfig,
                       scanner);

      mergeJob[area->GetType()->GetIndex()].areaCount++;

      // This is an area of a type that does not get merged,
      // we directly store it in the target file.
      if (!loadedTypes.IsSet(area->GetType())) {
        if (firstCall) {
          writer.Write(type);
          writer.Write(id);

          area->WriteImport(typeConfig,
                            writer);

          areasWritten++;
        }

        continue;
      }

      bool isMergeCandidate=false;

      for (const auto& ring: area->rings) {
        if (!ring.IsOuterRing()) {
          continue;
        }

        for (const auto node : ring.nodes) {
          if (nodeUseMap.find(node.GetId())!=nodeUseMap.end()) {
            isMergeCandidate=true;
            break;
          }
        }

        if (isMergeCandidate) {
          break;
        }
      }

      if (!isMergeCandidate) {
        continue;
      }

      if (mergeJob[area->GetType()->GetIndex()].areas.empty()) {
        typesWithAreas++;
      }

      mergeJob[area->GetType()->GetIndex()].areas.push_back(area);

      collectedAreasCount++;

      while (collectedAreasCount>parameter.GetRawWayBlockSize() &&
             typesWithAreas>1) {
        TypeInfoRef victimType;

        // Find the type with the smallest amount of ways loaded
        for (auto &type : loadedTypes) {
          if (!mergeJob[type->GetIndex()].areas.empty() &&
              (!victimType ||
               mergeJob[type->GetIndex()].areas.size()<mergeJob[victimType->GetIndex()].areas.size())) {
            victimType=type;
          }
        }

        // If there is more then one type of way, we always must find a "victim" type.
        assert(victimType);

        // Correct the statistics
        collectedAreasCount-=mergeJob[victimType->GetIndex()].areas.size();
        // Clear already loaded data of th victim type
        mergeJob[victimType->GetIndex()].areas.clear();

        typesWithAreas--;
        loadedTypes.Remove(victimType);
      }
    }

    progress.SetAction("Collected "+std::to_string(collectedAreasCount)+" areas for "+std::to_string(loadedTypes.Size())+" types");

    return true;
  }

  void MergeAreasGenerator::IndexAreasByNodeIds(const std::unordered_set<Id>& nodeUseMap,
                                                const std::list<AreaRef>& areas,
                                                std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (auto& area : areas) {
      std::unordered_set<Id> nodeIds;

      for (const auto& ring: area->rings) {
        if (ring.IsOuterRing()) {
          for (const auto node : ring.nodes) {
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

  bool MergeAreasGenerator::TryMerge(const std::unordered_set<Id>& nodeUseMap,
                                     Area& area,
                                     std::unordered_map<Id,std::set<AreaRef> >& idAreaMap,
                                     std::set<Id>& finishedIds,
                                     std::unordered_set<FileOffset>& mergedAway)
  {
    for (const auto& ring: area.rings) {
      if (!ring.IsOuterRing()) {
        continue;
      }

      std::unordered_set<Id> nodeIds;
      std::set<AreaRef>      visitedAreas; // It is possible that two areas intersect in multiple nodes, to avoid multiple merge tests, we monitor the visited areas

      for (const auto node : ring.nodes) {
        Id id=node.GetId();

        if(finishedIds.find(id)!=finishedIds.end()) {
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

        std::set<AreaRef>::iterator candidate=idAreaMap[id].begin();

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
            candidate++;
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

          if (area.rings[firstOuterRing].nodes.size()+candidateArea->rings[secondOuterRing].nodes.size()>FileWriter::MAX_NODES) {
            // We could merge, but we could not store the resulting area anymore
            ++candidate;
            continue;
          }

          // Flag as visited (the areas might be registered for other
          // nodes shared by both areas, too) and we do not want to
          // analyze it again
          visitedAreas.insert(candidateArea);

          // Areas do not have the same feature value buffer values, skip
          if (area.rings[firstOuterRing].GetFeatureValueBuffer()!=candidateArea->rings[secondOuterRing].GetFeatureValueBuffer()) {
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
            else {
              //std::cout << "COULD merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
            }
          }
          else {
            //std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << " at " << index << " " << node.GetCoord().GetDisplayText() << std::endl;
          }

          ++candidate;
        }

        finishedIds.insert(id);
      }
    }

    return false;
  }

  void MergeAreasGenerator::MergeAreas(Progress& progress,
                                       const std::unordered_set<Id>& nodeUseMap,
                                       AreaMergeData& job)
  {
    std::unordered_map<Id,std::set<AreaRef> > idAreaMap;
    size_t                                    overallCount=job.areas.size();

    IndexAreasByNodeIds(nodeUseMap,
                        job.areas,
                        idAreaMap);

    progress.Info("Found "+std::to_string(idAreaMap.size())+" nodes as possible connection points for areas");

    while (!job.areas.empty()) {
      AreaRef area;

      progress.SetProgress(overallCount-job.areas.size(),overallCount);

      // Pop a area from the list of "to be processed" areas
      area=job.areas.front();
      job.areas.pop_front();

      // This areas has already be "handled", ignore it
      if (job.mergedAway.find(area->GetFileOffset())!=job.mergedAway.end()) {
        continue;
      }

      // Delete all mentioning of this area in the idAreaMap cache
      EraseAreaInCache(nodeUseMap,
                       area,
                       idAreaMap);

      std::set<Id> finishedIds;

      // Now try to merge it against candidates that share the same ids.
      if (TryMerge(nodeUseMap,
                   *area,
                   idAreaMap,
                   finishedIds,
                   job.mergedAway)) {
        job.merges.push_back(area);

        while (TryMerge(nodeUseMap,
                        *area,
                        idAreaMap,
                        finishedIds,
                        job.mergedAway)) {
          // no code
        }
      }

    }
  }

  bool MergeAreasGenerator::WriteMergeResult(Progress& progress,
                                             const TypeConfig& typeConfig,
                                             FileScanner& scanner,
                                             FileWriter& writer,
                                             const TypeInfoSet& loadedTypes,
                                             std::vector<AreaMergeData>& mergeJob,
                                             uint32_t& areasWritten)
  {
    uint32_t                               areaCount=0;
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

    scanner.Read(areaCount);

    for (uint32_t a=1; a<=areaCount; a++) {
      uint8_t type;
      Id      id;
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,areaCount);

      scanner.Read(type);
      scanner.Read(id);

      area->ReadImport(typeConfig,
                       scanner);

      if (loadedTypes.IsSet(area->GetType())) {
        if (ignores.find(area->GetFileOffset())!=ignores.end()) {
          continue;
        }

        writer.Write(type);
        writer.Write(id);

        const auto& merge=merges.find(area->GetFileOffset()) ;

        if (merge!=merges.end()) {
          area=merge->second;
        }

        area->WriteImport(typeConfig,
                          writer);

        areasWritten++;
      }
    }

    return true;
  }

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

    std::unordered_set<Id> nodeUseMap;

    try {

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   MergeAreaDataGenerator::AREAS_TMP),
                   FileScanner::Sequential,
                   parameter.GetRawWayDataMemoryMaped());

      if (!ScanAreaNodeIds(progress,
                           *typeConfig,
                           scanner,
                           mergeTypes,
                           nodeUseMap)) {
        return false;
      }

      size_t   nodeCount=nodeUseMap.size();
      uint32_t areasWritten=0;

      progress.Info("Found "+std::to_string(nodeCount)+" nodes as possible connection points for areas");

      /* ------ */

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AREAS2_TMP));

      writer.Write(areasWritten);

      while (true) {
        TypeInfoSet                loadedTypes;
        std::vector<AreaMergeData> mergeJob(typeConfig->GetTypeCount());

        //
        // Load type data
        //

        progress.SetAction("Collecting area data by type");

        if (!GetAreas(parameter,
                      progress,
                      *typeConfig,
                      mergeTypes,
                      loadedTypes,
                      nodeUseMap,
                      scanner,
                      writer,
                      mergeJob,
                      areasWritten)) {
          return false;
        }

        // Merge

        progress.SetAction("Merging areas");

        for (const auto& type : loadedTypes) {
          if (!mergeJob[type->GetIndex()].areas.empty()) {
            progress.Info("Merging areas of type "+type->GetName());
            MergeAreas(progress,
                       nodeUseMap,
                       mergeJob[type->GetIndex()]);
            progress.Info("Reduced areas of '"+type->GetName()+"' from "+std::to_string(mergeJob[type->GetIndex()].areaCount)+" to "+std::to_string(mergeJob[type->GetIndex()].areaCount-mergeJob[type->GetIndex()].mergedAway.size()));

            mergeJob[type->GetIndex()].areas.clear();
          }
        }

        // Store back merge result

        if (!loadedTypes.Empty()) {
          if (!WriteMergeResult(progress,
                                *typeConfig,
                                scanner,
                                writer,
                                loadedTypes,
                                mergeJob,
                                areasWritten)) {
            return false;
          }

          mergeTypes.Remove(loadedTypes);
        }


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
