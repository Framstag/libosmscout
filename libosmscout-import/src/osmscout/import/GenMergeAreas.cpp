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
                                             std::unordered_map<Id,std::list<AreaRef> >& idAreaMap,
                                             const AreaRef& area)
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

  bool MergeAreasGenerator::ScanAreaIds(const ImportParameter& parameter,
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
          !data.Read(typeConfig,
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

  bool MergeAreasGenerator::GetAreas(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     TypeInfoSet& types,
                                     std::vector<NodeUseMap>& nodeUseMap,
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
      AreaRef area=new Area();

      progress.SetProgress(a,areaCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !area->Read(typeConfig,
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

  void MergeAreasGenerator::MergeAreas(const NodeUseMap& nodeUseMap,
                                       std::list<AreaRef>& areas,
                                       std::list<AreaRef>& /*merges*/,
                                       std::unordered_set<FileOffset>& blacklist)
  {
    std::unordered_map<Id,std::list<AreaRef> > idAreaMap;

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

          break;
        }
      }
    }

    while (!areas.empty()) {
      AreaRef area;

      area=areas.front();
      areas.pop_front();

      if (blacklist.find(area->GetFileOffset())!=blacklist.end()) {
        continue;
      }

      EraseAreaInCache(nodeUseMap,
                       idAreaMap,
                       area);
      bool merged;

      do {
        merged=false;

        for (const auto& ring: area->rings) {
          if (ring.ring!=Area::outerRingId) {
            continue;
          }

          std::unordered_set<Id> nodeIds;

          for (const auto id : ring.ids) {
            if (nodeIds.find(id)==nodeIds.end()) {
              nodeIds.insert(id);

              if (nodeUseMap.IsNodeUsedAtLeastTwice(id)) {

                std::list<AreaRef>::iterator candidate=idAreaMap[id].begin();

                while (!merged &&
                    candidate!=idAreaMap[id].end()) {
                  AreaRef candidateArea(*candidate);

                  size_t firstOuterRing=GetFirstOuterRingWithId(area,
                                                                id);

                  size_t secondOuterRing=GetFirstOuterRingWithId(candidateArea,
                                                                 id);

                  if (area->rings[firstOuterRing].GetFeatureValueBuffer()!=candidateArea->rings[secondOuterRing].GetFeatureValueBuffer()) {
                    //std::cout << "CANNOT merge areas " << area->GetFileOffset() << " and " << candidateArea->GetFileOffset() << " because of different feature values" << std::endl;
                    candidate++;
                    continue;
                  }

                  PolygonMerger merger;

                  merger.AddPolygon(area->rings[firstOuterRing].nodes,
                                    area->rings[firstOuterRing].ids);

                  merger.AddPolygon(candidateArea->rings[secondOuterRing].nodes,
                                    candidateArea->rings[secondOuterRing].ids);

                  std::list<PolygonMerger::Polygon> result;

                  if (merger.Merge(result)) {
                    if (result.size()==1) {
                      std::cout << "MERGE areas " << area->GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;

                      area->rings[firstOuterRing].nodes=result.front().coords;
                      area->rings[firstOuterRing].ids=result.front().ids;

                      EraseAreaInCache(nodeUseMap,
                                       idAreaMap,
                                       candidateArea);

                      blacklist.insert(candidateArea->GetFileOffset());

                      merged=true;
                    }
                    else {
                      std::cout << "COULD merge areas " << area->GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
                      candidate++;
                    }
                  }
                  else {
                    std::cout << "CANNOT merge areas " << area->GetFileOffset() << " and " << candidateArea->GetFileOffset() << std::endl;
                    candidate++;
                  }
                }
              }
            }
          }

          if (merged) {
            break;
          }
        }


      } while(merged);

    }
  }

  bool MergeAreasGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    return true;

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

    if (!ScanAreaIds(parameter,
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
