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
      if (area.rings[r].ring!=Area::outerRingId) {
        continue;
      }

      for (const auto& ringId : area.rings[r].ids) {
        if (ringId==id) {
          return r;
        }
      }
    }

    assert(false);

    return 0;
  }

  void MergeAreasGenerator::EraseAreaInCache(const NodeUseMap& nodeUseMap,
                                             const AreaRef& area,
                                             std::unordered_map<Id,std::list<AreaRef> >& idAreaMap)
  {
    std::unordered_set<Id> nodeIds;

    for (const auto& ring: area->rings) {
      if (ring.ring==Area::outerRingId) {
        for (const auto id: ring.ids) {
          if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
            std::remove(idAreaMap[id].begin(),
                        idAreaMap[id].end(),
                        area);
          }
        }

        break;
      }
    }
  }

  /**
   * Scans all areas. If an areas is of one of the given merge types, index all node ids
   * into the given NodeUseMap for all outer rings of the given area.
   */
  bool MergeAreasGenerator::ScanAreaNodeIds(const ImportParameter& parameter,
                                            Progress& progress,
                                            const TypeConfig& typeConfig,
                                            const TypeInfoSet& mergeTypes,
                                            std::vector<NodeUseMap>& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;

    progress.SetAction("Scanning ids from 'areas.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      uint8_t type;
      Id      id;
      Area    data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !data.ReadImport(typeConfig,
                           scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!mergeTypes.IsSet(data.GetType())) {
        continue;
      }

      for (const auto& ring: data.rings) {
        if (ring.ring!=Area::outerRingId) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto id : ring.ids) {
          if (nodeIds.find(id)==nodeIds.end()) {
            nodeUseMap[data.GetType()->GetIndex()].SetNodeUsed(id);
            nodeIds.insert(id);
          }
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
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
                                     TypeInfoSet& types,
                                     const std::vector<NodeUseMap>& nodeUseMap,
                                     FileScanner& scanner,
                                     std::vector<std::list<AreaRef> >& areas)
  {
    uint32_t    areaCount=0;
    size_t      collectedAreasCount=0;
    size_t      typesWithAreas=0;
    TypeInfoSet currentTypes(types);

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
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

      if (!currentTypes.IsSet(area->GetType())) {
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

      if (area->GetType()->GetName()=="landuse_allotments") {
        std::cout << area->GetFileOffset() << " => " << id << std::endl;
      }

      if (areas[area->GetType()->GetIndex()].empty()) {
        typesWithAreas++;
      }

      areas[area->GetType()->GetIndex()].push_back(area);

      collectedAreasCount++;

      while (collectedAreasCount>parameter.GetRawWayBlockSize() &&
             typesWithAreas>1) {
        TypeInfoRef victimType;

        // Find the type with the smallest amount of ways loaded
        for (auto &type : currentTypes) {
          if (!areas[type->GetIndex()].empty() &&
              (!victimType ||
               areas[type->GetIndex()].size()<areas[victimType->GetIndex()].size())) {
            victimType=type;
          }
        }

        // If there is more then one type of way, we always must find a "victim" type.
        assert(victimType);

        collectedAreasCount-=areas[victimType->GetIndex()].size();
        areas[victimType->GetIndex()].clear();

        typesWithAreas--;
        currentTypes.Remove(victimType);
      }
    }

    // If we are done, remove all successfully collected types from our list of "not yet collected" types.
    types.Remove(currentTypes);

    progress.SetAction("Collected "+NumberToString(collectedAreasCount)+" areas for "+NumberToString(currentTypes.Size())+" types");

    return true;
  }

  void MergeAreasGenerator::IndexAreasByNodeIds(const NodeUseMap& nodeUseMap,
                                                const std::list<AreaRef>& areas,
                                                std::unordered_map<Id,std::list<AreaRef> >& idAreaMap)
  {
    for (auto& area : areas) {
      for (const auto& ring: area->rings) {
        if (ring.ring==Area::outerRingId) {
          std::unordered_set<Id> nodeIds;

          for (const auto id: ring.ids) {
            if (nodeIds.find(id)==nodeIds.end() &&
                nodeUseMap.IsNodeUsedAtLeastTwice(id)) {
              idAreaMap[id].push_back(area);
              nodeIds.insert(id);
            }
          }
        }
      }
    }
  }

  bool MergeAreasGenerator::TryMerge(const NodeUseMap& nodeUseMap,
                                     Area& area,
                                     std::unordered_map<Id,std::list<AreaRef> >& idAreaMap,
                                     std::unordered_set<FileOffset>& blacklist)
  {
    for (const auto& ring: area.rings) {
      if (ring.ring!=Area::outerRingId) {
        continue;
      }

      std::unordered_set<Id> nodeIds;
      std::set<AreaRef>      visitedAreas; // It is possible that two areas intersect in multiple nodes, to avoid multiple merge tests, we monitor the visited areas

      for (const auto id : ring.ids) {
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

        std::list<AreaRef>::iterator candidate=idAreaMap[id].begin();

        while (candidate!=idAreaMap[id].end()) {
          AreaRef candidateArea(*candidate);

          // The area itself => skip
          if (area.GetFileOffset()==candidateArea->GetFileOffset()) {
            candidate++;
            continue;
          }

          // Already visited during this merge => skip
          if (visitedAreas.find(candidateArea)!=visitedAreas.end()) {
            candidate++;
            continue;
          }

          // Area was already merged before => skip
          if (blacklist.find(candidateArea->GetFileOffset())!=blacklist.end()) {
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
            std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << " because of different feature values" << std::endl;
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
              std::cout << "MERGE areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;

              area.rings[firstOuterRing].nodes=result.front().coords;
              area.rings[firstOuterRing].ids=result.front().ids;

              EraseAreaInCache(nodeUseMap,
                               candidateArea,
                               idAreaMap);

              blacklist.insert(candidateArea->GetFileOffset());

              return true;
            }
            else {
              std::cout << "COULD merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
              candidate++;
            }
          }
          else {
            std::cout << "CANNOT merge areas " << area.GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
            candidate++;
          }
        }
      }
    }

    return false;
  }

  void MergeAreasGenerator::MergeAreas(const NodeUseMap& nodeUseMap,
                                       std::list<AreaRef>& areas,
                                       std::list<AreaRef>& merges,
                                       std::unordered_set<FileOffset>& blacklist)
  {
    std::unordered_map<Id,std::list<AreaRef> > idAreaMap;

    IndexAreasByNodeIds(nodeUseMap,
                        areas,
                        idAreaMap);

    while (!areas.empty()) {
      AreaRef area;

      // Pop a area from the list of "to be processed" areas
      area=areas.front();
      areas.pop_front();

      // This areas has already be "handled", ignore it
      if (blacklist.find(area->GetFileOffset())!=blacklist.end()) {
        continue;
      }

      // Delete all mentioning of this area in the idAreaMap cache
      EraseAreaInCache(nodeUseMap,
                       area,
                       idAreaMap);

      // Now try to merge it against candidates that share the same ids.
      while (TryMerge(nodeUseMap,
                      *area,
                      idAreaMap,
                      blacklist)) {
        // no code
      };

    }
  }

  bool MergeAreasGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    TypeInfoSet                    mergeTypes;
    FileScanner                    scanner;
    uint32_t                       areaCount;
    std::unordered_set<FileOffset> blacklist;

    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->GetMergeAreas()) {
        mergeTypes.Set(type);
      }
    }

    std::vector<NodeUseMap> nodeUseMap;

    nodeUseMap.resize(typeConfig->GetTypeCount());

    if (!ScanAreaNodeIds(parameter,
                         progress,
                         *typeConfig,
                         mergeTypes,
                         nodeUseMap)) {
      return false;
    }

    for (const auto& type : typeConfig->GetTypes()) {
      if (type->GetMergeAreas()) {
        std::cout << type->GetName() << ": " << nodeUseMap[type->GetIndex()].GetNodeUsedCount() << std::endl;
      }
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                             "wayarea.tmp"),
                             FileScanner::Sequential,
                             parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    /* ------ */

    while (!mergeTypes.Empty()) {
      std::vector<std::list<AreaRef> > areasByType(typeConfig->GetTypeCount());

      //
      // Load type data
      //

      progress.SetAction("Collecting area data by type");

      if (!GetAreas(parameter,
                    progress,
                    *typeConfig,
                    mergeTypes,
                    nodeUseMap,
                    scanner,
                    areasByType)) {
        return false;
      }

      for (const auto& type : typeConfig->GetTypes()) {
        if (!areasByType[type->GetIndex()].empty()) {
          std::list<AreaRef> merges;

          std::cout << "Type " << type->GetName() << ": " << areasByType[type->GetIndex()].size() << " merge candidates" << std::endl;
          MergeAreas(nodeUseMap[type->GetIndex()],
                     areasByType[type->GetIndex()],
                     merges,
                     blacklist);

          areasByType[type->GetIndex()].clear();
        }
      }
    }

    return true;
  }
}
