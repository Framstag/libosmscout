/*
  TypeResolutionPerformanceTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

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

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/Tag.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/cli/CmdLineParsing.h>

/**
  Measures the cost of type resolution (GetNodeType, GetWayAreaType,
  GetRelationType) for a given type definition file (*.ost).

  The cost of type resolution scales with the number of types in the
  type definition, because the current implementation does a linear
  scan over all types and evaluates the tag conditions of each type
  until a match is found.

  Usage:
    TypeResolutionPerformanceTest [--iterations N] OST_FILE
*/

using namespace osmscout;

struct TagSet
{
  std::string name;
  std::vector<std::pair<std::string,std::string>> tags;
};

static TagMap MakeTagMap(TypeConfig& typeConfig,
                         const std::vector<std::pair<std::string,std::string>>& tags)
{
  TagMap tagMap;

  for (const auto& entry : tags) {
    tagMap[typeConfig.GetTagRegistry().RegisterTag(entry.first)]=entry.second;
  }

  return tagMap;
}

static void PrintResult(const std::string& label,
                        const StopClockNano& timer,
                        size_t iterations)
{
  double nsPerOp=timer.GetNanoseconds()/static_cast<double>(iterations);

  std::cout << "  " << label << ": "
            << nsPerOp/1000.0 << " us/op ("
            << timer.GetNanoseconds()/1000000.0 << " ms total, "
            << iterations << " iterations)"
            << std::endl;
}

static void RunNodeBenchmark(TypeConfig& typeConfig,
                             const std::vector<TagSet>& tagSets,
                             size_t iterations)
{
  std::cout << "GetNodeType:" << std::endl;

  for (const auto& tagSet : tagSets) {
    TagMap tagMap=MakeTagMap(typeConfig,
                             tagSet.tags);

    // Warmup
    for (size_t i=0; i<1000; i++) {
      typeConfig.GetNodeType(tagMap);
    }

    size_t        checksum=0;
    StopClockNano timer;

    for (size_t i=0; i<iterations; i++) {
      checksum+=typeConfig.GetNodeType(tagMap)->GetIndex();
    }

    timer.Stop();

    PrintResult(tagSet.name,
                timer,
                iterations);

    if (checksum==0) {
      std::cout << "    (no match)" << std::endl;
    }
  }
}

static void RunWayAreaBenchmark(TypeConfig& typeConfig,
                                const std::vector<TagSet>& tagSets,
                                size_t iterations)
{
  std::cout << "GetWayAreaType:" << std::endl;

  for (const auto& tagSet : tagSets) {
    TagMap tagMap=MakeTagMap(typeConfig,
                             tagSet.tags);

    // Warmup
    for (size_t i=0; i<1000; i++) {
      TypeInfoRef wayType;
      TypeInfoRef areaType;

      typeConfig.GetWayAreaType(tagMap,
                                wayType,
                                areaType);
    }

    size_t        checksum=0;
    StopClockNano timer;

    for (size_t i=0; i<iterations; i++) {
      TypeInfoRef wayType;
      TypeInfoRef areaType;

      typeConfig.GetWayAreaType(tagMap,
                                wayType,
                                areaType);

      checksum+=wayType->GetIndex()+areaType->GetIndex();
    }

    timer.Stop();

    PrintResult(tagSet.name,
                timer,
                iterations);

    if (checksum==0) {
      std::cout << "    (no match)" << std::endl;
    }
  }
}

static void RunRelationBenchmark(TypeConfig& typeConfig,
                                 const std::vector<TagSet>& tagSets,
                                 size_t iterations)
{
  std::cout << "GetRelationType:" << std::endl;

  for (const auto& tagSet : tagSets) {
    TagMap tagMap=MakeTagMap(typeConfig,
                             tagSet.tags);

    // Warmup
    for (size_t i=0; i<1000; i++) {
      typeConfig.GetRelationType(tagMap);
    }

    size_t        checksum=0;
    StopClockNano timer;

    for (size_t i=0; i<iterations; i++) {
      checksum+=typeConfig.GetRelationType(tagMap)->GetIndex();
    }

    timer.Stop();

    PrintResult(tagSet.name,
                timer,
                iterations);

    if (checksum==0) {
      std::cout << "    (no match)" << std::endl;
    }
  }
}

int main(int argc, char* argv[])
{
  using namespace std::string_literals;

  bool        help=false;
  size_t      iterations=100000;
  std::string ostFile;

  osmscout::CmdLineParser argParser("TypeResolutionPerformanceTest",
                                    argc,
                                    argv);

  argParser.AddOption(osmscout::CmdLineFlag([&](const bool& value) {
                        help=value;
                      }),
                      std::vector<std::string>{"h","help"},
                      "Display help",
                      true);

  argParser.AddOption(osmscout::CmdLineSizeTOption([&](const size_t& value) {
                        iterations=value;
                      }),
                      "iterations",
                      "Number of iterations per benchmark, default: "s+std::to_string(iterations));

  argParser.AddPositional(osmscout::CmdLineStringOption([&](const std::string& value) {
                            ostFile=value;
                          }),
                          "OST_FILE",
                          "Type definition file (*.ost)");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  if (ostFile.empty()) {
    std::cerr << "ERROR: No OST file given" << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  TypeConfigRef typeConfig=std::make_shared<TypeConfig>();

  if (!typeConfig->LoadFromOSTFile(ostFile)) {
    std::cerr << "ERROR: Cannot load OST file '" << ostFile << "'" << std::endl;
    return 1;
  }

  std::cout << "TypeConfig: " << typeConfig->GetTypes().size()
            << " types (" << typeConfig->GetNodeTypes().size() << " node, "
            << typeConfig->GetWayTypes().size() << " way, "
            << typeConfig->GetAreaTypes().size() << " area, "
            << typeConfig->GetRouteTypes().size() << " route)"
            << std::endl;
  std::cout << "Type ID bytes: node " << (int)typeConfig->GetNodeTypeIdBytes()
            << ", way " << (int)typeConfig->GetWayTypeIdBytes()
            << ", area " << (int)typeConfig->GetAreaTypeIdBytes()
            << std::endl;
  std::cout << std::endl;

  // Node tag sets: typical OSM node tag combinations
  std::vector<TagSet> nodeTagSets={
    {"highway=bus_stop (complex NOT condition)",
     {{"highway","bus_stop"},
      {"bus","yes"},
      {"public_transport","platform"},
      {"name","Main Street"}}},
    {"amenity=cafe + building=yes",
     {{"amenity","cafe"},
      {"building","yes"},
      {"name","Cafe Central"}}},
    {"shop=bakery",
     {{"shop","bakery"},
      {"name","Bakery"}}},
    {"building=yes (matches many building types)",
     {{"building","yes"}}},
    {"natural=water",
     {{"natural","water"}}},
    {"no match (full scan, registered keys)",
     {{"highway","foobar"},
      {"amenity","foobar"}}}
  };

  // Way/area tag sets
  std::vector<TagSet> wayAreaTagSets={
    {"highway=residential (way)",
     {{"highway","residential"},
      {"name","Main Street"}}},
    {"highway=track (way)",
     {{"highway","track"}}},
    {"shop=bakery (area, 377 shop conditions)",
     {{"shop","bakery"},
      {"name","Bakery"}}},
    {"amenity=school (area, 147 amenity conditions)",
     {{"amenity","school"},
      {"name","School"}}},
    {"leisure=park (area)",
     {{"leisure","park"},
      {"name","Park"}}},
    {"highway=motorway (way)",
     {{"highway","motorway"},
      {"ref","A1"}}},
    {"waterway=river (way)",
     {{"waterway","river"},
      {"name","Rhine"}}},
    {"landuse=residential (area)",
     {{"landuse","residential"}}},
    {"building=yes (area)",
     {{"building","yes"}}},
    {"no match (full scan, registered keys)",
     {{"highway","foobar"},
      {"amenity","foobar"}}}
  };

  // Relation tag sets
  std::vector<TagSet> relationTagSets={
    {"type=multipolygon + natural=water",
     {{"type","multipolygon"},
      {"natural","water"}}},
    {"type=route + route=bicycle",
     {{"type","route"},
      {"route","bicycle"},
      {"name","Tour de France"}}},
    {"type=restriction",
     {{"type","restriction"},
      {"restriction","no_left_turn"}}},
    {"no match (full scan, registered keys)",
     {{"type","foobar"},
      {"route","foobar"}}}
  };

  RunNodeBenchmark(*typeConfig,
                   nodeTagSets,
                   iterations);

  std::cout << std::endl;

  RunWayAreaBenchmark(*typeConfig,
                      wayAreaTagSets,
                      iterations);

  std::cout << std::endl;

  RunRelationBenchmark(*typeConfig,
                       relationTagSets,
                       iterations);

  return 0;
}
