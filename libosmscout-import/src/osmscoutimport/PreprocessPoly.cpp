/*
  This source is part of the libosmscout library
  Copyright (C) 2016 Lukas Karas

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

#include <osmscoutimport/PreprocessPoly.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

#include <osmscoutimport/RawNode.h>

namespace osmscout {

  PreprocessPoly::PreprocessPoly(PreprocessorCallback& callback)
  : callback(callback)
  {
  }

  bool PreprocessPoly::ClosePolygon(Progress& progress,
                                    Context context,
                                    std::list<GeoCoord>& polygonNodes,
                                    OSMId &availableId,
                                    TagId polygonTagId)
  {
    if (polygonNodes.size()>=2 &&
        polygonNodes.front()==polygonNodes.back()) {
      // last point is same as first, remove it
      polygonNodes.pop_back();
    }

    std::vector<GeoCoord> v1(polygonNodes.begin(), polygonNodes.end());

    if (!AreaIsSimple(v1)) {
      progress.Warning("Datapolygon is not simple!");
      return false;
    }

    std::vector<Node> nodes(polygonNodes.size());
    int               i=0;

    for (const auto& n : polygonNodes) {
      nodes[i].x=n.GetLon();
      nodes[i].y=n.GetLat();
      i++;
    }
    // undefined area should be on the right side
    bool ccw=AreaIsCCW(nodes);

    if ((context==IncludedPolygon && !ccw) || (context==ExcludedPolygon && ccw)) {
      progress.Debug("Reverse datapolygon");
      polygonNodes.reverse();
    }

    PreprocessorCallback::RawBlockDataRef block=std::make_shared<PreprocessorCallback::RawBlockData>();
    PreprocessorCallback::RawWayData      way;

    way.tags[polygonTagId]=context==IncludedPolygon ? "include" : "exclude";
    way.id=availableId++;

    for (const auto& p : polygonNodes) {
      PreprocessorCallback::RawNodeData node;
      OSMId                             id=availableId++;

      node.coord=p;
      node.id=id;
      way.nodes.push_back(id);
      block->nodeData.push_back(node);
    }

    // make sure that polygon is closed (area), add first node id as last
    way.nodes.push_back(way.id+1);

    block->wayData.push_back(way);

    callback.ProcessBlock(std::move(block));
    return true;
  }

  bool PreprocessPoly::Import(const TypeConfigRef& typeConfig,
                              const ImportParameter& parameter,
                              Progress& progress,
                              const std::string& filename)
  {
    progress.SetAction("Parsing *.poly file '{}'",filename);

    std::ifstream       infile(filename);
    std::string         line;
    Context             context=Root;
    std::string         fileName;
    std::string         sectionName;
    std::list<GeoCoord> polygonNodes;
    OSMId               availableId=parameter.GetFirstFreeOSMId();

    try {
      size_t lineNum=0;

      while (std::getline(infile, line)) {
        lineNum++;
        //std::cout << line << std::endl;
        switch(context) {
          case Root:
            if (line.length()>0) {
              context=Section;
              fileName=line;
            }
            break;

          case Section:
            if (line.length()>0) {
              if (line=="END") {
                context=Root;
              }
              else {
                if (line.substr(0,1)=="!") {
                  sectionName=line.substr(1);
                  context=ExcludedPolygon;
                  if (sectionName.empty()) {
                    throw IOException(filename, "Empty section name on line "+std::to_string(lineNum), "");
                  }
                }
                else {
                  sectionName=line;
                  context=IncludedPolygon;
                }
              }
            }
            break;

          case IncludedPolygon:
          case ExcludedPolygon:
            if (line=="END") {
              if (!ClosePolygon(progress, context, polygonNodes, availableId, typeConfig->tagDataPolygon)) {
                throw IOException(filename, "Invalid section on line "+std::to_string(lineNum), "");
              }
              context=Section;
              polygonNodes.clear();
              break;
            }

            std::list<std::string> tokens=SplitStringAtSpace(line);

            if (tokens.size()!=2) {
              throw IOException(filename, "Invalid format on line "+std::to_string(lineNum), "");
            }

            double lon;
            double lat;

            if (!StringToNumber(tokens.front(), lon)) {
              throw IOException(filename, "Invalid number format on line "+std::to_string(lineNum), "");
            }

            if (!StringToNumber(tokens.back(), lat)) {
              throw IOException(filename, "Invalid number format on line "+std::to_string(lineNum), "");
            }

            polygonNodes.emplace_back(lat, lon);
            break;
        }
      }

      infile.close();

      if (context!=Root) {
        throw IOException(filename, "Invalid polygon file.");
      }

      progress.SetProgress(1.0,1.0);
      return true;
    }
    catch (IOException& e) {
      infile.close();
      progress.Error(e.GetDescription());
      return false;
    }
  }
}
