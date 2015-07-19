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

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/DataFile.h>

namespace osmscout {

  std::string MergeAreasGenerator::GetDescription() const
  {
    return "Merge areas";
  }

  /**
   * Returns the index of the first outer ring that contains the given id.
   */
  size_t MergeAreasGenerator::GetFirstOuterRingWithId(const Area& area,
                                                      Id id) const
  {
    for (size_t r = 0; r<area.rings.size(); r++) {
      if (area.rings[r].ring==Area::outerRingId) {
        for (const auto& ringId : area.rings[r].ids) {
          if (ringId==id) {
            return r;
          }
        }
      }
    }

    assert(false);

    return 0;
  }

  void MergeAreasGenerator::EraseAreaInCache(const NodeUseMap& nodeUseMap,
                                             const AreaRef& area,
                                             std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (const auto& ring: area->rings) {
      if (ring.ring==Area::outerRingId) {
        for (const auto id : ring.ids) {
          if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
            idAreaMap[id].erase(area);
          }
        }
      }
    }
  }

  void MergeAreasGenerator::EraseAreaInCache(Id currentId,
                                             const NodeUseMap& nodeUseMap,
                                             const AreaRef& area,
                                             std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (const auto& ring: area->rings) {
      if (ring.ring==Area::outerRingId) {
        for (const auto id : ring.ids) {
          if (id==currentId) {
            continue;
          }

          if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
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
                                            std::vector<NodeUseMap>& nodeUseMap)
  {
    uint32_t    areaCount=0;

    progress.SetAction("Scanning for nodes joining areas from '"+scanner.GetFilename()+"'");

    if (!scanner.GotoBegin()) {
      progress.Error(std::string("Cannot position to start of file '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=areaCount; current++) {
      uint8_t type;
      Id      id;
      Area    data;

      progress.SetProgress(current,areaCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !data.ReadImport(typeConfig,
                           scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!mergeTypes.IsSet(data.GetType())) {
        continue;
      }

      // We insert every node id only once per area, because we want to
      // find nodes that are shared by *different* areas.

      std::unordered_set<Id> nodeIds;

      for (const auto& ring: data.rings) {
        if (ring.ring==Area::outerRingId) {
          for (const auto id : ring.ids) {
            if (nodeIds.find(id)==nodeIds.end()) {
              nodeUseMap[data.GetType()->GetIndex()].SetNodeUsed(id);
              nodeIds.insert(id);
            }
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
                                     const std::vector<NodeUseMap>& nodeUseMap,
                                     FileScanner& scanner,
                                     FileWriter& writer,
                                     std::vector<MergeJob>& mergeJob,
                                     uint32_t& areasWritten)
  {
    uint32_t    areaCount=0;
    size_t      collectedAreasCount=0;
    size_t      typesWithAreas=0;

    loadedTypes=candidateTypes;

    if (!scanner.GotoBegin()) {
      progress.Error(std::string("Cannot position to start of file '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t a=1; a<=areaCount; a++) {
      uint8_t type;
      Id      id;
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,areaCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !area->ReadImport(typeConfig,
                            scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(a)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!loadedTypes.IsSet(area->GetType())) {
        if (!(writer.Write(type) &&
              writer.Write(id) &&
              area->WriteImport(typeConfig,
                                writer))) {
          progress.Error(std::string("Error while writing ")+
                         " area to file '"+
                         writer.GetFilename()+"'");

          return false;
        }

        areasWritten++;

        continue;
      }

      bool isMergeCandidate=false;

      for (const auto& ring: area->rings) {
        if (ring.ring!=Area::outerRingId) {
          continue;
        }

        for (const auto id: ring.ids) {
          if (nodeUseMap[area->GetType()->GetIndex()].IsNodeUsedAtLeastTwice(id)) {
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

    progress.SetAction("Collected "+NumberToString(collectedAreasCount)+" areas for "+NumberToString(loadedTypes.Size())+" types");

    return true;
  }

  void MergeAreasGenerator::IndexAreasByNodeIds(const NodeUseMap& nodeUseMap,
                                                const std::list<AreaRef>& areas,
                                                std::unordered_map<Id,std::set<AreaRef> >& idAreaMap)
  {
    for (auto& area : areas) {
      std::unordered_set<Id> nodeIds;

      for (const auto& ring: area->rings) {
        if (ring.ring==Area::outerRingId) {
          for (const auto id: ring.ids) {
            if (nodeIds.find(id)==nodeIds.end() &&
                nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
              idAreaMap[id].insert(area);
              nodeIds.insert(id);
            }
          }
        }
      }
    }
  }

  bool MergeAreasGenerator::TryMerge(const NodeUseMap& nodeUseMap,
                                     Area& area,
                                     std::unordered_map<Id,std::set<AreaRef> >& idAreaMap,
                                     std::set<Id>& finishedIds,
                                     std::unordered_set<FileOffset>& mergedAway)
  {
    for (const auto& ring: area.rings) {
      if (ring.ring!=Area::outerRingId) {
        continue;
      }

      std::unordered_set<Id> nodeIds;
      std::set<AreaRef>      visitedAreas; // It is possible that two areas intersect in multiple nodes, to avoid multiple merge tests, we monitor the visited areas

      for (const auto id : ring.ids) {
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
        if (!nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
          continue;
        }

        // We now have an node id other areas with the same node id exist for.
        // Let's take a look at all these candidates

        std::set<AreaRef>::iterator candidate=idAreaMap[id].begin();

        while (candidate!=idAreaMap[id].end()) {
          AreaRef candidateArea(*candidate);

          // The area itself => skip
          if (area.GetFileOffset()==candidateArea->GetFileOffset()) {
            // TODO: Should not happen?
            candidate++;
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
            candidate++;
            continue;
          }

          // Flag as visited (the areas might be registered for other
          // nodes shared by both areas, too) and we not want to
          // analyze it again
          visitedAreas.insert(candidateArea);

          size_t firstOuterRing=GetFirstOuterRingWithId(area,
                                                        id);

          size_t secondOuterRing=GetFirstOuterRingWithId(*candidateArea,
                                                         id);

          if (area.rings[firstOuterRing].GetFeatureValueBuffer()!=candidateArea->rings[secondOuterRing].GetFeatureValueBuffer()) {
            //std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << " because of different feature values" << std::endl;
            candidate++;
            continue;
          }

          // Yes, we can merge!

          PolygonMerger merger;

          merger.AddPolygon(area.rings[firstOuterRing].nodes,
                            area.rings[firstOuterRing].ids);

          merger.AddPolygon(candidateArea->rings[secondOuterRing].nodes,
                            candidateArea->rings[secondOuterRing].ids);

          std::list<PolygonMerger::Polygon> result;

          if (merger.Merge(result)) {
            if (result.size()==1) {
              //std::cout << "MERGE areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;

              area.rings[firstOuterRing].nodes=result.front().coords;
              area.rings[firstOuterRing].ids=result.front().ids;

              EraseAreaInCache(id,
                               nodeUseMap,
                               candidateArea,
                               idAreaMap);

              mergedAway.insert(candidateArea->GetFileOffset());

              return true;
            }
            else {
              //std::cout << "COULD merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
              candidate++;
            }
          }
          else {
            //std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
            candidate++;
          }
        }

        finishedIds.insert(id);
      }
    }

    return false;
  }

  void MergeAreasGenerator::MergeAreas(Progress& progress,
                                       const NodeUseMap& nodeUseMap,
                                       MergeJob& job)
  {
    std::unordered_map<Id,std::set<AreaRef> > idAreaMap;
    size_t                                    overallCount=job.areas.size();

    IndexAreasByNodeIds(nodeUseMap,
                        job.areas,
                        idAreaMap);

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
                                             std::vector<MergeJob>& mergeJob,
                                             uint32_t& areasWritten)
  {
    uint32_t                               areaCount=0;
    std::unordered_map<FileOffset,AreaRef> merges;
    std::unordered_set<FileOffset>         ignores;
    /*
    size_t      collectedAreasCount=0;
    size_t      typesWithAreas=0;*/

    for (const auto& type : loadedTypes) {
      for (const auto& area : mergeJob[type->GetIndex()].areas) {
        merges[area->GetFileOffset()]=area;
      }

      ignores.insert(mergeJob[type->GetIndex()].mergedAway.begin(),
                     mergeJob[type->GetIndex()].mergedAway.end());
    }

    if (!scanner.GotoBegin()) {
      progress.Error(std::string("Cannot position to start of file '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t a=1; a<=areaCount; a++) {
      uint8_t type;
      Id      id;
      AreaRef area=std::make_shared<Area>();

      progress.SetProgress(a,areaCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !area->ReadImport(typeConfig,
                            scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(a)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (loadedTypes.IsSet(area->GetType())) {
        if (ignores.find(area->GetFileOffset())!=ignores.end()) {
          continue;
        }

        if (!(writer.Write(type) &&
              writer.Write(id))) {
          return false;
        }

        const auto& merge=merges.find(area->GetFileOffset()) ;

        if (merge!=merges.end()) {
          area=merge->second;
        }

        if (!area->WriteImport(typeConfig,
                                writer)) {
          progress.Error(std::string("Error while writing ")+
                         " area to file '"+
                         writer.GetFilename()+"'");

          return false;
        }

        areasWritten++;
      }
    }

    return true;
  }

  bool MergeAreasGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    TypeInfoSet                    mergeTypes;
    FileScanner                    scanner;
    FileWriter                     writer;
    uint32_t                       areasWritten=0;

    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->GetMergeAreas()) {
        mergeTypes.Set(type);
      }
    }

    std::vector<NodeUseMap> nodeUseMap;

    nodeUseMap.resize(typeConfig->GetTypeCount());

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.tmp"),
                             FileScanner::Sequential,
                             parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areas2.tmp"))) {
      progress.Error("Cannot create '" + writer.GetFilename() + "'");
      return false;
    }

    writer.Write(areasWritten);

    if (!ScanAreaNodeIds(progress,
                         *typeConfig,
                         scanner,
                         mergeTypes,
                         nodeUseMap)) {
      return false;
    }

    /* ------ */

    while (!mergeTypes.Empty()) {
      TypeInfoSet           loadedTypes;
      std::vector<MergeJob> mergeJob(typeConfig->GetTypeCount());

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

      for (const auto& type : loadedTypes) {
        if (!mergeJob[type->GetIndex()].areas.empty()) {
          size_t                          candidateCount=mergeJob[type->GetIndex()].areas.size();

          progress.SetAction("Merging Type "+type->GetName());
          MergeAreas(progress,
                     nodeUseMap[type->GetIndex()],
                     mergeJob[type->GetIndex()]);
          progress.Info(NumberToString(candidateCount)+" candidate(s), "+NumberToString(mergeJob[type->GetIndex()].merges.size())+" merges");

          mergeJob[type->GetIndex()].areas.clear();
        }
      }

      // Store back merge result

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

    if (!(writer.GotoBegin() &&
          writer.Write(areasWritten))) {
      progress.Error(std::string("Error while writing number of entries to file '")+
                     writer.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     writer.GetFilename()+"'");
      return false;
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    return true;
  }
}
