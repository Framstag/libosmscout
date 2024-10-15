/*
  DumpData - a demo program for libosmscout
  Copyright (C) 2012  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <osmscout/cli/CmdLineParsing.h>

#include <osmscout/db/Database.h>
#include <osmscout/db/DebugDatabase.h>

#include <osmscout/routing/RouteNode.h>

#include <osmscout/routing/RouteNodeDataFile.h>
#include <osmscout/routing/RoutingService.h>

#include <osmscout/description/DescriptionService.h>

/*
 * Example:
 *   src/DumpData ../TravelJinni/ -n 25293125 -w 4290108 -w 26688152 -r 531985
 */

struct Job
{
  osmscout::ObjectOSMRef  osmRef;
  osmscout::ObjectFileRef fileRef;

  Job() = default;

  Job(osmscout::OSMRefType type, osmscout::OSMId id)
  : osmRef(id,type)
  {
    // no code
  }

  Job(osmscout::RefType type, osmscout::FileOffset fileOffset)
  : fileRef(fileOffset,type)
  {
    // no code
  }
};

struct Arguments
{
  std::string               map;
  std::set<osmscout::OSMId> coordIds;
  std::set<osmscout::OSMId> routeNodeCoordIds;
  std::set<osmscout::Id>    routeNodeIds;
  std::list<Job>            jobs;
  bool                      help;
};

static const size_t INDENT=2;

static void DumpIndent(size_t indent)
{
  assert(indent<=10);
  for (size_t i=1; i<=indent; i++) {
    std::cout << " ";
  }
}

static void DumpPoint(osmscout::OSMId osmId,
                      const osmscout::Point& point)
{

  std::cout << "Point {" << std::endl;
  std::cout << "  OSMId: " << osmId << std::endl;
  std::cout << "  Serial: " << point.GetSerial() << std::endl;
  std::cout << "  OSMScoutId: " << point.GetId() << std::endl;

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  std::cout << "  lat: " << point.GetCoord().GetLat() << std::endl;
  std::cout << "  lon: " << point.GetCoord().GetLon() << std::endl;

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  std::cout << "}" << std::endl;
}

static void DumpRouteNode(const osmscout::RouteNode& routeNode)
{
  std::cout << "RouteNode {" << std::endl;
  std::cout << "  fileOffset: " << routeNode.GetFileOffset() << std::endl;
  std::cout << "  RouteNodeId: " << routeNode.GetId() << std::endl;

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  std::cout << "  lat: " << routeNode.GetCoord().GetLat() << std::endl;
  std::cout << "  lon: " << routeNode.GetCoord().GetLon() << std::endl;

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  for (const auto& object : routeNode.objects) {
    std::cout << std::endl;
    std::cout << "  object {" << std::endl;
    std::cout << "    object: " << object.object.GetName() << std::endl;
    std::cout << "    variant: " << object.objectVariantIndex << std::endl;
    std::cout << "  }" << std::endl;
  }

  for (const auto& path : routeNode.paths) {
    std::cout << std::endl;
    std::cout << "  path {" << std::endl;
    std::cout << "    object: " << routeNode.objects[path.objectIndex].object.GetName() << std::endl;
    std::cout << "  }" << std::endl;
  }

  for (const auto& exclude : routeNode.excludes) {
    std::cout << std::endl;
    std::cout << "  exclude {" << std::endl;
    std::cout << "    from: " << exclude.source.GetName() << std::endl;
    std::cout << "    to: " << routeNode.objects[exclude.targetIndex].object.GetName() << std::endl;
    std::cout << "  }" << std::endl;
  }

  std::cout << "}" << std::endl;
}

static void DumpDescription(const osmscout::ObjectDescription& description,
                            size_t indent)
{
  std::string previousSection;
  std::string previousSubsection;
  size_t      previousIndex=std::numeric_limits<size_t>::max();

  for (const auto& entry : description.GetEntries()) {
    // Close previous subsection, if subsection changes
    if (!previousSubsection.empty() &&
      (entry.GetSubsectionKey()!=previousSubsection || !entry.HasSubsection())) {
      assert(indent>=2);
      indent-=2;
      DumpIndent(indent);
      std::cout << "}" << std::endl;
    }

    // Close previous section, if section changes
    if (!previousSection.empty() &&
      entry.GetSectionKey()!=previousSection) {
      assert(indent>=2);
      indent-=2;
      DumpIndent(indent);
      std::cout << "}" << std::endl;
    }

    if (entry.GetSectionKey()!=previousSection) {
      if (!previousSection.empty()) {
        std::cout << std::endl;
      }

      DumpIndent(indent);
      std::cout << entry.GetSectionKey() << " {" << std::endl;
      indent+=2;
    }

    if (entry.HasSubsection() && entry.GetSubsectionKey()!=previousSubsection) {
      DumpIndent(indent);
      std::cout << entry.GetSubsectionKey() << " {" << std::endl;
      indent+=2;
    }


    if (entry.HasIndex() && entry.GetIndex()>1 && entry.GetIndex()!=previousIndex) {
      assert(indent>=2);
      DumpIndent(indent-2);
      std::cout << "}" << std::endl;
      DumpIndent(indent-2);
      std::cout << entry.GetSubsectionKey() << " {" << std::endl;
    }

    previousSection=entry.GetSectionKey();
    previousSubsection=entry.GetSubsectionKey();

    if (entry.HasIndex()) {
      previousIndex=entry.GetIndex();
    }
    else {
      previousIndex=std::numeric_limits<size_t>::max();
    }

    DumpIndent(indent);
    std::cout << entry.GetLabelKey() << ": ";
    std::cout << entry.GetValue() << std::endl;
  }

  if (!previousSubsection.empty()) {
    assert(indent>=2);
    indent-=2;
    DumpIndent(indent);
    std::cout << "}" << std::endl;
  }

  if (!previousSection.empty()) {
    assert(indent>=2);
    indent-=2;
    DumpIndent(indent);
    std::cout << "}" << std::endl;
  }
}


static void DumpNode(const osmscout::DescriptionService& descriptionService,
                     const osmscout::Node& node,
                     osmscout::OSMId id)
{
  osmscout::ObjectDescription description=descriptionService.GetDescription(node);

  std::cout << "Node {" << std::endl;
  std::cout << "  OSM id: " << id << std::endl;
  std::cout << std::endl;
  DumpDescription(description,2);

  std::cout << std::endl;

  std::cout << "  lat: " << node.GetCoords().GetLat() << std::endl;
  std::cout << "  lon: " << node.GetCoords().GetLon() << std::endl;

  std::cout << "}" << std::endl;
}

static void DumpWay(const osmscout::DescriptionService& descriptionService,
                     const osmscout::Way& way,
                     osmscout::OSMId id)
{
  osmscout::ObjectDescription description=descriptionService.GetDescription(way);

  std::cout << "Way {" << std::endl;
  std::cout << "  OSM id: " << id << std::endl;
  std::cout << std::endl;
  DumpDescription(description,2);

  if (!way.nodes.empty()) {
    std::cout << std::endl;

    for (size_t n=0; n<way.nodes.size(); n++) {
      std::cout << "  node[" << n << "] {";

      if (way.GetSerial(n)!=0) {
        std::cout << " serial: " << way.GetSerial(n);
        std::cout << " id: " << way.GetId(n);
      }

      std::cout << " lat: " << way.GetCoord(n).GetLat() << " lon: "<< way.GetCoord(n).GetLon() << " }" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

static void DumpArea(const osmscout::DescriptionService& descriptionService,
                     const osmscout::Area& area,
                     osmscout::OSMId id)
{
  osmscout::ObjectDescription description=descriptionService.GetDescription(area);

  std::cout << "Area {" << std::endl;
  std::cout << "  OSM id: " << id << std::endl;
  std::cout << std::endl;

  DumpDescription(description,2);

  for (size_t r=0; r<area.rings.size(); r++) {
    std::cout << std::endl;

    size_t indent;
    if (area.rings[r].IsMaster()) {
      indent=INDENT;
    }
    else {
      std::cout << "  role[" << r << "] {" << std::endl;
      indent=INDENT+2;
    }

    if (area.rings[r].IsMaster()) {
      DumpIndent(indent);
      std::cout << "role: master" << std::endl;
    }
    else if (area.rings[r].IsTopOuter()) {
      DumpIndent(indent);
      std::cout << "role: outer" << std::endl;
    }
    else {
      DumpIndent(indent);
      std::cout << "ring: " << (size_t)area.rings[r].GetRing() << std::endl;
    }

    if (!area.rings[r].nodes.empty()) {
      osmscout::GeoBox boundingBox=area.rings[r].GetBoundingBox();
      DumpIndent(indent);
      std::cout << "boundingBox: " << boundingBox.GetDisplayText() << std::endl;
      DumpIndent(indent);
      std::cout << "center of bounding box: " << boundingBox.GetCenter().GetDisplayText() << std::endl;

      if (area.rings[r].center) {
        DumpIndent(indent);
        std::cout << "visual center: " << area.rings[r].center.value().GetDisplayText() << std::endl;
      }
    }

    osmscout::ObjectDescription rinDescription=descriptionService.GetDescription(area.rings[r].GetFeatureValueBuffer());

    DumpDescription(rinDescription,indent);

    if (!area.rings[r].nodes.empty()) {
      std::cout << std::endl;

      for (size_t n=0; n<area.rings[r].nodes.size(); n++) {
        DumpIndent(indent);
        std::cout << "node[" << n << "] {";

        if (area.rings[r].GetSerial(n)!=0) {
          std::cout << "serial: " << area.rings[r].GetSerial(n);
        }

        std::cout << " lat: " << area.rings[r].nodes[n].GetLat() << " lon: "<< area.rings[r].nodes[n].GetLon() << " }" << std::endl;
      }
    }

    if (!area.rings[r].IsMaster()) {
      indent-=2;
      DumpIndent(indent);
      std::cout << "}" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

osmscout::CmdLineParseResult ParseArguments(int argc, char** argv, Arguments& args)
{
  osmscout::CmdLineParser argParser("DumpData",
                                    argc, argv);
  std::vector<std::string> helpArgs{"h", "help"};

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.map=value;
                          }),
                          "map",
                          "Libosmscout map directory");

  argParser.AddOption(osmscout::CmdLineInt64TOption([&args](const int64_t& value) {
                        args.coordIds.insert(value);
                      }),
                      "c",
                      "OSM coord id");

  argParser.AddOption(osmscout::CmdLineInt64TOption([&args](const int64_t& value) {
                        args.jobs.emplace_back(osmscout::osmRefNode, value);
                      }),
                      "n",
                      "OSM node id");

  argParser.AddOption(osmscout::CmdLineInt64TOption([&args](const int64_t& value) {
                        args.jobs.emplace_back(osmscout::osmRefWay, value);
                      }),
                      "w",
                      "OSM way id");

  argParser.AddOption(osmscout::CmdLineInt64TOption([&args](const int64_t& value) {
                        args.jobs.emplace_back(osmscout::osmRefRelation, value);
                      }),
                      "r",
                      "OSM relation id");

  argParser.AddOption(osmscout::CmdLineInt64TOption([&args](const int64_t& value) {
                        args.routeNodeCoordIds.insert(value);
                      }),
                      "rn",
                      "OSM routing node id");

  argParser.AddOption(osmscout::CmdLineUInt64TOption([&args](const uint64_t& value) {
                        args.jobs.emplace_back(osmscout::refNode, value);
                      }),
                      "no",
                      "Libosmscout node fileoffset");

  argParser.AddOption(osmscout::CmdLineUInt64TOption([&args](const uint64_t& value) {
                        args.jobs.emplace_back(osmscout::refWay, value);
                      }),
                      "wo",
                      "Libosmscout way fileoffset");

  argParser.AddOption(osmscout::CmdLineUInt64TOption([&args](const uint64_t& value) {
                        args.jobs.emplace_back(osmscout::refArea, value);
                      }),
                      "ao",
                      "Libosmscout area fileoffset");

  argParser.AddOption(osmscout::CmdLineUInt64TOption([&args](const uint64_t& value) {
                        args.routeNodeIds.insert(value);
                      }),
                      "ri",
                      "Libosmscout route node id");

  osmscout::CmdLineParseResult result=argParser.Parse();
  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
  }
  else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
  }

  return result;
}

int main(int argc, char* argv[])
{
  // Try to initialize current locale

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error&) {
    std::cerr << "ERROR: Cannot set locale" << std::endl;
  }

  Arguments args;

  if (osmscout::CmdLineParseResult result=ParseArguments(argc, argv, args);
    result.HasError()) {
    return 1;
  }

  if (args.help) {
    return 0;
  }

  osmscout::DatabaseParameter      databaseParameter;
  osmscout::Database               database(databaseParameter);
  osmscout::DebugDatabaseParameter debugDatabaseParameter;
  osmscout::DebugDatabase          debugDatabase(debugDatabaseParameter);

  osmscout::RouteNodeDataFile routeNodeDataFile(osmscout::RoutingService::GetDataFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
                                                1000);

  osmscout::DescriptionService descriptionService;

  if (!database.Open(args.map)) {
    std::cerr << "Cannot open db" << std::endl;
  }

  if (!debugDatabase.Open(args.map)) {
    std::cerr << "Cannot open debug db" << std::endl;
  }

  if (!routeNodeDataFile.Open(database.GetTypeConfig(),
                              args.map,
                              true)) {
    std::cerr << "Cannot open routing db" << std::endl;
  }

  // OSM ids
  std::set<osmscout::ObjectOSMRef>  osmRefs;
  std::set<osmscout::ObjectFileRef> fileRefs;

  for (const auto& job : args.jobs) {
    switch (job.osmRef.GetType()) {
    case osmscout::osmRefNone:
      break;
    case osmscout::osmRefNode:
    case osmscout::osmRefWay:
    case osmscout::osmRefRelation:
      osmRefs.insert(job.osmRef);
      break;
    }

    switch (job.fileRef.GetType()) {
    case osmscout::refNone:
      break;
    case osmscout::refNode:
    case osmscout::refArea:
    case osmscout::refWay:
      fileRefs.insert(job.fileRef);
      break;
    }
  }

  std::multimap<osmscout::ObjectOSMRef,osmscout::ObjectFileRef> idFileOffsetMap;
  std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef> fileOffsetIdMap;

  if (!osmRefs.empty() ||
      !fileRefs.empty()) {
    if (!debugDatabase.ResolveReferences(osmRefs,
                                         fileRefs,
                                         idFileOffsetMap,
                                         fileOffsetIdMap)) {
      std::cerr << "Error while resolving node ids and file offsets" << std::endl;
    }
  }

  osmscout::CoordDataFile::ResultMap coordsMap;
  std::vector<osmscout::NodeRef>     nodes;
  std::vector<osmscout::AreaRef>     areas;
  std::vector<osmscout::WayRef>      ways;

  osmscout::CoordDataFile::ResultMap                      routeCoordsMap;
  std::unordered_map<osmscout::Id,osmscout::RouteNodeRef> routeNodeMap;

  //
  // Load data
  //

  if (!args.coordIds.empty()) {

    if (!debugDatabase.GetCoords(args.coordIds,
                                 coordsMap)) {
      std::cerr << "Error whole loading coords by id" << std::endl;
    }
  }

  if (!args.routeNodeCoordIds.empty()) {

    if (!debugDatabase.GetCoords(args.routeNodeCoordIds,
                                 routeCoordsMap)) {
      std::cerr << "Error whole loading route node coords by id" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator entry=fileOffsetIdMap.begin();
         entry!=fileOffsetIdMap.end();
         ++entry) {
      if (entry->first.GetType()==osmscout::refNode) {
        offsets.push_back(entry->first.GetFileOffset());
      }
    }

    if (!database.GetNodesByOffset(offsets,
                                   nodes)) {
      std::cerr << "Error whole loading nodes by offset" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator entry=fileOffsetIdMap.begin();
         entry!=fileOffsetIdMap.end();
         ++entry) {
      if (entry->first.GetType()==osmscout::refArea) {
        offsets.push_back(entry->first.GetFileOffset());
      }
    }

    if (!database.GetAreasByOffset(offsets,
                                   areas)) {
      std::cerr << "Error whole loading areas by offset" << std::endl;
    }
  }

  if (!fileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (const auto& entry : fileOffsetIdMap) {
      if (entry.first.GetType()==osmscout::refWay) {
        offsets.push_back(entry.first.GetFileOffset());
      }
    }

    if (!database.GetWaysByOffset(offsets,
                                  ways)) {
      std::cerr << "Error whole loading ways by offset" << std::endl;
    }
  }

  for (const auto id : args.routeNodeCoordIds) {
    auto coordsEntry=routeCoordsMap.find(id);

    if (coordsEntry!=routeCoordsMap.end()) {
      args.routeNodeIds.insert(coordsEntry->second.GetId());
    }
    else {
      std::cerr << "Cannot find route node coord with id " << id << std::endl;
    }
  }

  if (!args.routeNodeIds.empty() &&
      routeNodeDataFile.IsOpen()) {
    for (const osmscout::Id id : args.routeNodeIds){
      osmscout::RouteNodeRef node;
      if (!routeNodeDataFile.Get(id,node)) {
        std::cerr << "Error loading route nodes by id" << std::endl;
        continue;
      }
      if (!node){
        std::cerr << "Error loading route nodes by id" << std::endl;
        continue;
      }
      routeNodeMap[id]=std::move(node);
    }
  }

  //
  // Start of dump
  //

  size_t dumpCounter=0;

  for (const auto id : args.coordIds) {
    auto coordsEntry=coordsMap.find(id);

    if (coordsEntry!=coordsMap.end()) {
      if (dumpCounter>0) {
        std::cout << "---" << std::endl;
      }

      DumpPoint(coordsEntry->first,
                coordsEntry->second);
      dumpCounter++;
    }
    else {
      std::cerr << "Cannot find coord with id " << id << std::endl;
    }
  }

  for (const auto id : args.routeNodeIds) {
    auto routeNodeEntry=routeNodeMap.find(id);

    if (routeNodeEntry!=routeNodeMap.end()) {
      if (dumpCounter>0) {
        std::cout << "---" << std::endl;
      }

      DumpRouteNode(*routeNodeEntry->second);
      dumpCounter++;
    }
    else {
      std::cerr << "Cannot find route node with id " << id << std::endl;
    }
  }

  std::streamsize         oldPrecision=std::cout.precision(5);
  std::ios_base::fmtflags oldFlags=std::cout.setf(std::ios::fixed,std::ios::floatfield);

  for (std::list<Job>::const_iterator job=args.jobs.begin();
       job!=args.jobs.end();
       ++job) {
    if (dumpCounter>0) {
      std::cout << "---" << std::endl;
    }

    if (job->osmRef.GetType()!=osmscout::osmRefNone) {
      std::map<osmscout::ObjectOSMRef,osmscout::ObjectFileRef>::const_iterator reference=idFileOffsetMap.lower_bound(job->osmRef);

      if (reference==idFileOffsetMap.end()) {
        std::cerr << "Cannot find '" << job->osmRef.GetTypeName() << "' with id " << job->osmRef.GetId() << std::endl;
        continue;
      }

      for (; reference!=idFileOffsetMap.upper_bound(job->osmRef); ++reference) {
        switch (reference->second.GetType()) {
          case osmscout::refNone:
            break;
          case osmscout::refNode:
            for (const auto &node : nodes) {
              if (reference->second.GetFileOffset() == node->GetFileOffset()) {
                DumpNode(descriptionService, *node, reference->first.GetId());
                dumpCounter++;
                break;
              }
            }
            break;
          case osmscout::refArea:
            for (const auto &area : areas) {
              if (reference->second.GetFileOffset() == area->GetFileOffset()) {
                DumpArea(descriptionService,*area, reference->first.GetId());
                dumpCounter++;
                break;
              }
            }
            break;
          case osmscout::refWay:
            for (const auto &way : ways) {
              if (reference->second.GetFileOffset() == way->GetFileOffset()) {
                DumpWay(descriptionService, *way, reference->first.GetId());
                dumpCounter++;
                break;
              }
            }
            break;
        }
      }
    }
    else if (job->fileRef.GetType()!=osmscout::refNone) {
      std::map<osmscout::ObjectFileRef,osmscout::ObjectOSMRef>::const_iterator reference=fileOffsetIdMap.find(job->fileRef);

      if (reference==fileOffsetIdMap.end()) {
        std::cerr << "Cannot find '" << job->fileRef.GetTypeName() << "' with offset " << job->fileRef.GetFileOffset() << std::endl;
        continue;
      }

      switch (reference->first.GetType()) {
      case osmscout::refNone:
        break;
      case osmscout::refNode:
        for (const auto& node : nodes) {
          if (reference->first.GetFileOffset()==node->GetFileOffset()) {
            DumpNode(descriptionService,*node,reference->second.GetId());
            dumpCounter++;
            break;
          }
        }
        break;
      case osmscout::refArea:
        for (const auto& area : areas) {
          if (reference->first.GetFileOffset()==area->GetFileOffset()) {
            DumpArea(descriptionService,*area, reference->second.GetId());
            dumpCounter++;
            break;
          }
        }
        break;
      case osmscout::refWay:
        for (const auto& way : ways) {
          if (reference->first.GetFileOffset()==way->GetFileOffset()) {
            DumpWay(descriptionService, *way, reference->second.GetId());
            dumpCounter++;
            break;
          }
        }
        break;
      }
    }
  }

  std::cout.setf(oldFlags,std::ios::floatfield);
  std::cout.precision(oldPrecision);

  routeNodeDataFile.Close();

  database.Close();

  debugDatabase.Close();

  return 0;
}
