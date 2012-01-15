/*
 This source is part of the libosmscout library
 Copyright (C) 2011  Tim Teulings

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

#include <osmscout/import/GenOptimizeLowZoom.h>

#include <cassert>

#include <osmscout/Way.h>

#include <osmscout/Util.h>

#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/private/Math.h>

#include <iostream>

namespace osmscout
{
  std::string OptimizeLowZoomGenerator::GetDescription() const
  {
    return "Generate 'optimize.dat'";
  }

  void OptimizeLowZoomGenerator::GetTypesToOptimize(const TypeConfig& typeConfig,
                                                    std::set<TypeId>& types)
  {
    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
        type!=typeConfig.GetTypes().end();
        type++) {
      if (type->GetOptimizeLowZoom()) {
        types.insert(type->GetId());
      }
    }
  }

  void OptimizeLowZoomGenerator::WriteHeader(FileWriter& writer,
                                             const std::set<TypeId>& types,
                                             size_t optimizeMaxMap,
                                             std::map<TypeId,FileOffset>& typeOffsetMap)
  {
    uint32_t typeCount=types.size();

    writer.WriteNumber((uint32_t)optimizeMaxMap);
    writer.Write(typeCount);

    for (std::set<TypeId>::const_iterator type=types.begin();
        type!=types.end();
        type++) {
      FileOffset currentOffset;
      uint32_t   count=0;
      FileOffset offset=0;

      writer.Write(*type);

      writer.GetPos(currentOffset);
      typeOffsetMap[*type]=currentOffset;

      writer.Write(count);
      writer.Write(offset);
    }
  }

  bool OptimizeLowZoomGenerator::GetWaysToOptimize(Progress& progress,
                                                   FileScanner& scanner,
                                                   TypeId type,
                                                   std::list<WayRef>& ways)
  {
    size_t nodes=0;
    size_t wayCount=0;

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      WayRef way=new Way();

      progress.SetProgress(w,wayCount);

      if (!way->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
            NumberToString(w)+" of "+
            NumberToString(wayCount)+
            " in file '"+
            scanner.GetFilename()+"'");
        return false;
      }

      if (way->GetType()==type && way->nodes.size()>=2) {
        ways.push_back(way);
        nodes+=way->nodes.size();
      }
    }

    progress.Info(NumberToString(ways.size())+ " ways, " + NumberToString(nodes)+ " nodes");

    return true;
  }

  void OptimizeLowZoomGenerator::MergeWays(const std::list<WayRef>& ways,
                                           std::list<WayRef>& newWays)
  {
    std::map<Id, std::list<WayRef > > waysByJoin;
    std::set<Id>                      usedWays;

    for (std::list<WayRef>::const_iterator way=ways.begin();
        way!=ways.end();
        way++) {
      waysByJoin[(*way)->nodes.front().GetId()].push_back(*way);
      waysByJoin[(*way)->nodes.back().GetId()].push_back(*way);
    }

    for (std::map<Id, std::list<WayRef> >::iterator entry=waysByJoin.begin();
        entry!=waysByJoin.end();
        entry++) {
      while (!entry->second.empty()) {

        WayRef way=entry->second.front();

        entry->second.pop_front();

        if (usedWays.find(way->GetId())!=usedWays.end()) {
          continue;
        }

        //std::cout << "Joining way " << way->GetId() << " " << way->GetName() << " " << way->GetRefName() << std::endl;
        usedWays.insert(way->GetId());

        while (true) {
          std::map<Id, std::list<WayRef> >::iterator match;

          match=waysByJoin.find(way->nodes.front().GetId());
          if (match!=waysByJoin.end())
          {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetId())!=usedWays.end() ||
                    way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetId())!=usedWays.end() ||
                      way->GetRefName()!=(*stillOtherWay)->GetRefName())) {
                stillOtherWay++;
              }

              // If we have at least three ways with the same joining node
              // we do not merge because we have a multi-way junctions
              if (stillOtherWay!=match->second.end()) {
                otherWay=match->second.end();
              }
            }

            if (otherWay!=match->second.end()) {
              std::vector<Point> newNodes;

              //std::cout << "...with way " << (*otherWay)->GetId() << " " << (*otherWay)->GetName() << " " << (*otherWay)->GetRefName() << std::endl;

              newNodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->nodes.front().GetId()==(*otherWay)->nodes.front().GetId()) {
                for (size_t i=(*otherWay)->nodes.size()-1; i>0; i--) {
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=0; i<way->nodes.size(); i++) {
                  newNodes.push_back(way->nodes[i]);
                }

                way->nodes=newNodes;
              }
              else {
                for (size_t i=0; i<(*otherWay)->nodes.size(); i++) {
                  newNodes.push_back((*otherWay)->nodes[i]);
                }

                for (size_t i=1; i<way->nodes.size(); i++) {
                  newNodes.push_back(way->nodes[i]);
                }

                way->nodes=newNodes;
              }

              usedWays.insert((*otherWay)->GetId());
              match->second.erase(otherWay);

              continue;
            }
          }

          match=waysByJoin.find(way->nodes.back().GetId());
          if (match!=waysByJoin.end())
          {
            std::list<WayRef>::iterator otherWay;

            // Search for matching way that has the same endpoint, the same ref name (and is not the way itself)
            otherWay=match->second.begin();
            while (otherWay!=match->second.end() &&
                   (usedWays.find((*otherWay)->GetId())!=usedWays.end() ||
                     way->GetRefName()!=(*otherWay)->GetRefName())) {
              otherWay++;
            }

            // Search for another way with the same criteria (because then we would have a multi-junction)
            if (otherWay!=match->second.end()) {
              std::list<WayRef>::iterator stillOtherWay=otherWay;

              stillOtherWay++;
              while (stillOtherWay!=match->second.end() &&
                     (usedWays.find((*stillOtherWay)->GetId())!=usedWays.end() ||
                      way->GetRefName()!=(*stillOtherWay)->GetRefName())) {
                stillOtherWay++;
              }

              // If we have at least three ways with the same joining node
              // we do not merge because we have a multi-way junctions
              if (stillOtherWay!=match->second.end()) {
                otherWay=match->second.end();
              }
            }

            if (otherWay!=match->second.end()) {
              //std::cout << "...with way " << (*otherWay)->GetId() << " " << (*otherWay)->GetName() << " " << (*otherWay)->GetRefName() << std::endl;

              way->nodes.reserve(way->nodes.size()+(*otherWay)->nodes.size()-1);

              if (way->nodes.back().GetId()==(*otherWay)->nodes.front().GetId()) {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  way->nodes.push_back((*otherWay)->nodes[i]);
                }
              }
              else {
                for (size_t i=1; i<(*otherWay)->nodes.size(); i++) {
                  size_t idx=(*otherWay)->nodes.size()-1-i;

                  way->nodes.push_back((*otherWay)->nodes[idx]);
                }
              }

              usedWays.insert((*otherWay)->GetId());
              match->second.erase(otherWay);

              continue;
            }
          }

          break;
        }

        newWays.push_back(*way);
      }
    }
  }

  bool OptimizeLowZoomGenerator::OptimizeWriteWays(Progress& progress,
                                                   FileWriter& writer,
                                                   FileOffset typeFileOffset,
                                                   const std::list<WayRef>& newWays,
                                                   size_t width,
                                                   size_t height,
                                                   double magnification)
  {
    size_t            newNodesCount=0;
    MercatorProjection projection;
    FileOffset        dataOffset;

    writer.GetPos(dataOffset);

    writer.SetPos(typeFileOffset);

    writer.Write((uint32_t)newWays.size());
    writer.Write(dataOffset);

    writer.SetPos(dataOffset);

    projection.Set(0,0,magnification,width,height);

    for (std::list<WayRef>::const_iterator way=newWays.begin();
        way!=newWays.end();
        way++) {
      TransPolygon       polygon;
      std::vector<Point> newNodes;

      polygon.TransformWay(projection,true,(*way)->nodes);

      newNodes.reserve(polygon.GetLength());

      for (size_t i=polygon.GetStart(); i<=polygon.GetEnd(); i++) {
        if (polygon.points[i].draw) {
          newNodes.push_back((*way)->nodes[i]);
        }
      }

      (*way)->nodes=newNodes;

      if (!(*way)->Write(writer)) {
        return false;
      }

      newNodesCount+=(*way)->nodes.size();
    }

    progress.Info(NumberToString(newNodesCount)+ " nodes");

    return true;
  }

  bool OptimizeLowZoomGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    std::set<TypeId>            types;         // Types we optimize
    FileScanner                 wayScanner;
    FileWriter                  writer;
    std::map<TypeId,FileOffset> dataOffsets;   // FileOffset where we store the file offset the the type data
    double                      magnification; // Magnification, we optimize for

    magnification=pow(2.0,(int)parameter.GetOptimizationMaxMag());

    progress.SetAction("Getting types to optimize");

    GetTypesToOptimize(typeConfig,types);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "optimized.dat"))) {
      progress.Error("Cannot create 'relations.dat'");
      return false;
    }

    //
    // Write header
    //

    WriteHeader(writer,
                types,
                parameter.GetOptimizationMaxMag(),
                dataOffsets);

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
        "ways.dat"))) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    for (std::set<TypeId>::const_iterator type=types.begin();
         type!=types.end();
         type++) {
      std::list<WayRef>                           ways;
      std::list<WayRef>                           newWays;
      std::map<TypeId,FileOffset>::const_iterator dataOffsetOffset;

      dataOffsetOffset=dataOffsets.find(*type);

      assert(dataOffsetOffset!=dataOffsets.end());

      //
      // Load type data
      //

      progress.SetAction("Scanning ways for type '" + typeConfig.GetTypeInfo(*type).GetName() + "' to gather data to optimize");


      if (!GetWaysToOptimize(progress,
                             wayScanner,
                             *type,
                             ways))
      {
        return false;
      }

      //
      // Join ways
      //

      progress.SetAction("Merging ways");

      MergeWays(ways,newWays);

      ways.clear();

      //
      // Transform/Optimize the way and store it
      //

      progress.Info(NumberToString(newWays.size())+ " ways");

      if (!OptimizeWriteWays(progress,
                             writer,
                             dataOffsetOffset->second,
                             newWays,
                             800,640,
                             magnification)) {
        return false;
      }
    }

    return !wayScanner.HasError() && wayScanner.Close();
  }
}

