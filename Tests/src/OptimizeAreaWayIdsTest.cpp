/*
  OptimizeAreaWayIds - the rule that decides which node serials the optimized area and way data keep

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

/*
 * Tests for the rule that decides which node serials the optimized area and way data keep:
 *
 *  - the rule itself, driven with the shapes that are rare in real data (an id twice inside one
 *    ring, an id twice inside a non-circular way, an id shared between an area and a way, the id a
 *    circular way returns to, an id only referenced by objects that cannot route) - these need no
 *    file and no pipeline,
 *  - the cost of the rule, which has to follow the number of distinct ids and not the number of
 *    objects it is fed with; this is pinned with a heap allocation counter, not with a duration,
 *    because a duration on a shared machine does not resolve it,
 *  - the files the step provides, which have to be byte-identical to the files the previous
 *    implementation provides for the same input; the test keeps that implementation as a reference
 *    and compares the two outputs byte for byte.
 */

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ios>
#include <iterator>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>
#include <osmscout/util/PolygonCenter.h>
#include <osmscout/util/Progress.h>

#include <osmscoutimport/GenOptimizeAreaWayIds.h>
#include <osmscoutimport/ImportParameter.h>

#include <osmscoutimport/private/AreaWayIdReferenceRule.h>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define ID_RULE_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define ID_RULE_HAVE_ALLOCATION_COUNTER 0
#else
#define ID_RULE_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define ID_RULE_HAVE_ALLOCATION_COUNTER 1
#endif

#if ID_RULE_HAVE_ALLOCATION_COUNTER

namespace {

  /**
   * Counts every heap allocation of the test process (the same approach as
   * Tests/src/MapPainterAreaPreparationTest.cpp). The property this pins is that the rule
   * allocates per object or not, which a duration cannot resolve on a shared machine.
   * Aligned allocations are not counted.
   */
  std::atomic<size_t> allocationCounter{0};

  size_t AllocationCount()
  {
    return allocationCounter.load(std::memory_order_relaxed);
  }
}

/*
 * The replaced operators pair malloc with free. GCC reports that pairing as a mismatched
 * new/delete (-Wmismatched-new-delete) at the free call of the sized delete once the objects the
 * test builds are inlined in the same translation unit, so the diagnostic is disabled around the
 * operators (the same suppression as in Tests/src/MapPainterLabelReuseTest.cpp).
 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

void* operator new(std::size_t size)
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  void * memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void* operator new[](std::size_t size)
{
  return ::operator new(size);
}

void* operator new(std::size_t size,
                   const std::nothrow_t&) noexcept
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  return std::malloc(size);
}

void* operator new[](std::size_t size,
                     const std::nothrow_t& tag) noexcept
{
  return ::operator new(size,tag);
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     const std::nothrow_t&) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       const std::nothrow_t&) noexcept
{
  std::free(memory);
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif

namespace {

  using namespace osmscout;

  /*
   * The names of the temporary files the step reads. They come from the generators that provide
   * them, whose classes the shared library does not export, so they are repeated here - and the
   * repetition is self-checking: if either side drifts, the step does not find the input this
   * test writes and the test fails. The names of the files the step provides are taken from the
   * step itself.
   */
  const char* const AREAS2_TMP="areas2.tmp";
  const char* const WAYWAY_TMP="wayway.tmp";

  /**
   * Builds the point of one logical node. The id the rule decides about is derived from the
   * coordinate and the serial (Point::GetId()), so the logical node number selects a coordinate
   * and every logical node carries the same serial.
   */
  Point Node(Id logicalNode)
  {
    double lat=52.0+(double)(logicalNode%1000)*0.00001;
    double lon=7.0+std::floor((double)logicalNode/1000.0)*0.00001;

    return Point{1,
                 GeoCoord(lat,
                          lon)};
  }

  std::vector<Point> Nodes(std::initializer_list<Id> logicalNodes)
  {
    std::vector<Point> nodes;

    for (const auto logicalNode : logicalNodes) {
      nodes.push_back(Node(logicalNode));
    }

    return nodes;
  }

  bool Keeps(const std::vector<Point>& nodes,
             size_t index)
  {
    return nodes[index].GetSerial()!=0;
  }

  // ---------------------------------------------------------------------------
  // The rule itself
  // ---------------------------------------------------------------------------

  TEST_CASE("An id one object references does not keep its serial")
  {
    AreaWayIdReferenceRule rule;

    REQUIRE(rule.AddObject(Nodes({1,2,3}))==3);

    REQUIRE_FALSE(rule.KeepsSerial(Node(1).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(2).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(3).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==3);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==0);
  }

  TEST_CASE("An id two objects reference keeps its serial")
  {
    AreaWayIdReferenceRule rule;

    rule.AddObject(Nodes({1,2}));
    rule.AddObject(Nodes({2,3}));

    REQUIRE_FALSE(rule.KeepsSerial(Node(1).GetId()));
    REQUIRE(rule.KeepsSerial(Node(2).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(3).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==3);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==1);
  }

  TEST_CASE("An id shared between an area and a way keeps its serial")
  {
    // The rule does not care which kind of object references an id, and the step feeds the
    // areas before the ways, so the later way has to see the area's reference.
    AreaWayIdReferenceRule rule;

    rule.AddObject(Nodes({10,11}));
    rule.AddObject(Nodes({11,12}));

    REQUIRE(rule.KeepsSerial(Node(11).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(12).GetId()));
  }

  TEST_CASE("An id repeated inside one ring counts as referenced once")
  {
    AreaWayIdReferenceRule rule;

    // A ring that visits the same node twice can still be closed without the id.
    REQUIRE(rule.AddObject(Nodes({1,2,1}))==2);

    REQUIRE_FALSE(rule.KeepsSerial(Node(1).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(2).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==2);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==0);
  }

  TEST_CASE("An id repeated inside one non-circular way counts as referenced once")
  {
    AreaWayIdReferenceRule rule;

    REQUIRE(rule.AddObject(Nodes({1,2,1,3}))==3);

    REQUIRE_FALSE(rule.KeepsSerial(Node(1).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==3);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==0);
  }

  TEST_CASE("The id a circular way returns to keeps its serial")
  {
    AreaWayIdReferenceRule rule;

    REQUIRE(rule.AddObject(Nodes({20,21,22}))==3);

    rule.ForceSerial(Node(20).GetId());

    REQUIRE(rule.KeepsSerial(Node(20).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(21).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(22).GetId()));
  }

  TEST_CASE("An id only objects that cannot route reference is not referenced")
  {
    // The step never feeds a ring or a way whose type cannot route, so an id that only such
    // objects carry is unknown to the rule and its serial is cleared.
    AreaWayIdReferenceRule rule;

    rule.AddObject(Nodes({30,31}));

    REQUIRE_FALSE(rule.KeepsSerial(Node(30).GetId()));
    REQUIRE_FALSE(rule.KeepsSerial(Node(31).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==2);
  }

  TEST_CASE("The distinct count of an object counts its ids once")
  {
    AreaWayIdReferenceRule rule;

    REQUIRE(rule.AddObject(Nodes({40,40,41,41,41,42}))==3);
    REQUIRE(rule.AddObject(Nodes({}))==0);
  }

  TEST_CASE("The decision does not depend on the number of objects")
  {
    AreaWayIdReferenceRule rule;

    rule.AddObject(Nodes({50,51}));
    rule.AddObject(Nodes({51,52}));

    REQUIRE(rule.KeepsSerial(Node(51).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==3);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==1);

    // The per-object distinctness is not carried to the next object.
    rule.AddObject(Nodes({53,53}));

    REQUIRE_FALSE(rule.KeepsSerial(Node(53).GetId()));
    REQUIRE(rule.GetReferencedIdCount()==4);
  }

#if ID_RULE_HAVE_ALLOCATION_COUNTER

  TEST_CASE("Deciding the same ids for many objects allocates a constant amount")
  {
    AreaWayIdReferenceRule   rule;

    const std::vector<Point> ids=Nodes({60,61,62,63,64,65,66,67,68,69});

    // First object: this one may allocate, it grows the scratch buffer and the sets.
    REQUIRE(rule.AddObject(ids)==10);

    const size_t before=AllocationCount();

    for (size_t i=0; i<1000; i++) {
      rule.AddObject(ids);
    }

    const size_t forThousand=AllocationCount()-before;

    for (size_t i=0; i<10000; i++) {
      rule.AddObject(ids);
    }

    const size_t forTenThousand=AllocationCount()-before-forThousand;

    // Ten times the objects may not cost more allocations than the thousand before them. The
    // implementation this change replaced allocated one container per object, so this failed
    // with ten thousand against one thousand.
    INFO("allocations for 1000 objects: " << forThousand
                                          << ", for 10000 objects: " << forTenThousand);
    REQUIRE(forTenThousand<=forThousand);

    REQUIRE(rule.GetReferencedIdCount()==10);
    REQUIRE(rule.GetUsedAtLeastTwiceCount()==10);
  }

#endif

  // ---------------------------------------------------------------------------
  // The files the step provides
  // ---------------------------------------------------------------------------

  struct TestTypes
  {
    TypeConfigRef typeConfig;
    TypeInfoRef   routableAreaType;
    TypeInfoRef   plainAreaType;
    TypeInfoRef   routableWayType;
    TypeInfoRef   plainWayType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<TypeConfig>();

    types.routableAreaType=std::make_shared<TypeInfo>("test_area_routable");
    types.routableAreaType->CanBeArea(true);
    types.routableAreaType->CanRouteFoot(true);
    types.typeConfig->RegisterType(types.routableAreaType);

    types.plainAreaType=std::make_shared<TypeInfo>("test_area_plain");
    types.plainAreaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.plainAreaType);

    types.routableWayType=std::make_shared<TypeInfo>("test_way_routable");
    types.routableWayType->CanBeWay(true);
    types.routableWayType->CanRouteCar(true);
    types.typeConfig->RegisterType(types.routableWayType);

    types.plainWayType=std::make_shared<TypeInfo>("test_way_plain");
    types.plainWayType->CanBeWay(true);
    types.typeConfig->RegisterType(types.plainWayType);

    return types;
  }

  Area MakeArea(const TypeInfoRef& type,
                const std::vector<Point>& nodes)
  {
    Area       area;
    Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);
    ring.nodes=nodes;

    area.rings.push_back(ring);

    return area;
  }

  Way MakeWay(const TypeInfoRef& type,
              const std::vector<Point>& nodes)
  {
    Way way;

    way.SetType(type);
    way.nodes=nodes;

    return way;
  }

  /**
   * An L-shaped ring. Its pole of inaccessibility is far enough from its bounding box centre
   * that the step records the centre instead of dropping it, which the two implementations
   * have to agree about byte for byte.
   */
  std::vector<Point> LShapedRing(Id firstLogicalNode)
  {
    std::vector<Point> nodes;

    nodes.emplace_back(1,GeoCoord(52.0000,7.0000));
    nodes.emplace_back(1,GeoCoord(52.0000,7.0030));
    nodes.emplace_back(1,GeoCoord(52.0010,7.0030));
    nodes.emplace_back(1,GeoCoord(52.0010,7.0010));
    nodes.emplace_back(1,GeoCoord(52.0030,7.0010));
    nodes.emplace_back(1,GeoCoord(52.0030,7.0000));

    // The logical node numbers are not part of the geometry; the ring above is fixed, so the
    // parameter only keeps the call sites readable.
    (void)firstLogicalNode;

    return nodes;
  }

  void WriteAreas2(const TestTypes& types,
                   const std::string& directory,
                   const std::vector<Area>& areas)
  {
    FileWriter writer;

    writer.Open(AppendFileToDir(directory,
                                AREAS2_TMP));

    writer.Write((uint32_t)areas.size());

    for (size_t i=0; i<areas.size(); i++) {
      // The step passes the leading type byte and the object id through without looking at them.
      writer.Write((uint8_t)0);
      writer.Write((Id)(i+1));

      areas[i].WriteImport(*types.typeConfig,
                           writer);
    }

    writer.Close();
  }

  void WriteWayWay(const TestTypes& types,
                   const std::string& directory,
                   const std::vector<Way>& ways)
  {
    FileWriter writer;

    writer.Open(AppendFileToDir(directory,
                                WAYWAY_TMP));

    writer.Write((uint32_t)ways.size());

    for (size_t i=0; i<ways.size(); i++) {
      writer.Write((uint8_t)0);
      writer.Write((Id)(i+1));

      ways[i].Write(*types.typeConfig,
                    writer);
    }

    writer.Close();
  }

  /**
   * The rule as the step implemented it before this change: a container per ring and per way,
   * folded into the two global sets.
   */
  class ReferenceRule
  {
  private:
    std::unordered_set<Id> referencedIds;
    std::unordered_set<Id> usedAtLeastTwiceIds;

  public:
    void AddObject(const std::vector<Point>& nodes)
    {
      std::unordered_set<Id> nodeIds;

      for (const auto& node : nodes) {
        nodeIds.insert(node.GetId());
      }

      for (const auto id : nodeIds) {
        if (referencedIds.contains(id)) {
          usedAtLeastTwiceIds.insert(id);
        }
        else {
          referencedIds.insert(id);
        }
      }
    }

    void ForceSerial(Id id)
    {
      usedAtLeastTwiceIds.insert(id);
    }

    bool KeepsSerial(Id id) const
    {
      return usedAtLeastTwiceIds.contains(id);
    }
  };

  /**
   * The centre rule of the copy phases, as they implemented it before this change.
   */
  std::optional<GeoCoord> ReferenceRingCenter(const Area::Ring& ring)
  {
    if (ring.nodes.empty()) {
      return std::nullopt;
    }

    auto   bbox=ring.GetBoundingBox();
    double dimension=std::max(bbox.GetWidth(),bbox.GetHeight());
    auto   center=PolygonCenter(ring.nodes,
                                dimension*0.01);

    auto bboxCenter=bbox.GetCenter();
    auto a=bboxCenter.GetLat()-center.GetLat();
    auto b=bboxCenter.GetLon()-center.GetLon();

    if (std::sqrt(a*a+b*b)<dimension*0.2) {
      return std::nullopt;
    }

    return center;
  }

  bool ReferenceDecide(const TestTypes& types,
                       const std::string& directory,
                       ReferenceRule& rule)
  {
    {
      FileScanner scanner;

      scanner.Open(AppendFileToDir(directory,
                                   AREAS2_TMP),
                   FileScanner::Sequential,
                   false);

      uint32_t areaCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=areaCount; current++) {
        Area area;

        (void)scanner.ReadUInt8();
        (void)scanner.ReadInt64();

        area.ReadImport(*types.typeConfig,
                        scanner);

        for (const auto& ring : area.rings) {
          if (!ring.GetType()->CanRoute()) {
            continue;
          }

          rule.AddObject(ring.nodes);
        }
      }

      scanner.Close();
    }

    {
      FileScanner scanner;

      scanner.Open(AppendFileToDir(directory,
                                   WAYWAY_TMP),
                   FileScanner::Sequential,
                   false);

      uint32_t wayCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=wayCount; current++) {
        Way way;

        (void)scanner.ReadUInt8();
        (void)scanner.ReadInt64();

        way.Read(*types.typeConfig,
                 scanner);

        if (!way.GetType()->CanRoute()) {
          continue;
        }

        rule.AddObject(way.nodes);

        if (way.IsCircular()) {
          rule.ForceSerial(way.GetBackId());
        }
      }

      scanner.Close();
    }

    return true;
  }

  /**
   * The copy phases as the step implemented them before this change, writing the reference files
   * into their own directory so that the two outputs can be compared.
   */
  bool ReferenceCopyAreas(const TestTypes& types,
                          const std::string& sourceDirectory,
                          const std::string& targetDirectory,
                          const ReferenceRule& rule)
  {
    FileScanner scanner;
    FileWriter  writer;

    scanner.Open(AppendFileToDir(sourceDirectory,
                                 AREAS2_TMP),
                 FileScanner::Sequential,
                 false);

    uint32_t areaCount=scanner.ReadUInt32();

    writer.Open(AppendFileToDir(targetDirectory,
                                OptimizeAreaWayIdsGenerator::AREAS3_TMP));

    writer.Write(areaCount);

    for (uint32_t current=1; current<=areaCount; current++) {
      Area    area;

      uint8_t type=scanner.ReadUInt8();
      OSMId   osmId=scanner.ReadInt64();

      area.ReadImport(*types.typeConfig,
                      scanner);

      for (auto& ring : area.rings) {
        for (auto& node : ring.nodes) {
          if (!rule.KeepsSerial(node.GetId())) {
            node.ClearSerial();
          }
        }

        ring.center=ReferenceRingCenter(ring);
      }

      writer.Write(type);
      writer.Write(osmId);

      // The step provides areas3.tmp in the database format (the step that reads it, the area
      // index generator, reads it with Area::Read), so the reference writes it the same way -
      // including the ring centre the two implementations have to agree about.
      area.Write(*types.typeConfig,
                 writer);
    }

    scanner.Close();
    writer.Close();

    return true;
  }

  bool ReferenceCopyWays(const TestTypes& types,
                         const std::string& sourceDirectory,
                         const std::string& targetDirectory,
                         const ReferenceRule& rule)
  {
    FileScanner scanner;
    FileWriter  writer;

    scanner.Open(AppendFileToDir(sourceDirectory,
                                 WAYWAY_TMP),
                 FileScanner::Sequential,
                 false);

    uint32_t wayCount=scanner.ReadUInt32();

    writer.Open(AppendFileToDir(targetDirectory,
                                OptimizeAreaWayIdsGenerator::WAYS_TMP));

    writer.Write(wayCount);

    for (uint32_t current=1; current<=wayCount; current++) {
      Way     way;

      uint8_t type=scanner.ReadUInt8();
      OSMId   osmId=scanner.ReadInt64();

      way.Read(*types.typeConfig,
               scanner);

      for (auto& node : way.nodes) {
        if (!rule.KeepsSerial(node.GetId())) {
          node.ClearSerial();
        }
      }

      writer.Write(type);
      writer.Write(osmId);

      way.Write(*types.typeConfig,
                writer);
    }

    scanner.Close();
    writer.Close();

    return true;
  }

  std::string ReadBinary(const std::filesystem::path& path)
  {
    std::ifstream file(path,
                       std::ios::binary);

    const std::string content{std::istreambuf_iterator<char>(file),
                              std::istreambuf_iterator<char>()};

    return content;
  }

  std::string HexDump(const std::string& data,
                      size_t limit=240)
  {
    std::string result;

    for (size_t i=0; i<std::min(data.size(),limit); i++) {
      static const char* const digits="0123456789abcdef";

      uint8_t                  value=(uint8_t)data[i];

      result+=' ';
      result+=digits[(value>>4u) & 0x0fu];
      result+=digits[value & 0x0fu];
    }

    return result;
  }

  void RequireSameBytes(const std::filesystem::path& expected,
                        const std::filesystem::path& actual)
  {
    const std::string left=ReadBinary(expected);
    const std::string right=ReadBinary(actual);

    INFO("reference file " << expected.string() << " has " << left.size() << " bytes, "
                           << "the step's file " << actual.string() << " has " << right.size());

    if (left.size()!=right.size()) {
      INFO("reference bytes:" << HexDump(left));
      INFO("step bytes:     " << HexDump(right));
    }

    REQUIRE(left.size()==right.size());

    for (size_t i=0; i<left.size(); i++) {
      if (left[i]!=right[i]) {
        INFO("first difference at byte " << i);
        REQUIRE(left[i]==right[i]);
      }
    }
  }

  std::filesystem::path MakeFixtureDirectory(const std::string& name)
  {
    const char            * env=getenv("TESTS_TMP_DIR");

    std::filesystem::path base=env!=nullptr
                                 ? std::filesystem::path(env)
                                 : std::filesystem::temp_directory_path();

    std::filesystem::path directory=base/("optimize-area-way-ids-"+name);

    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    return directory;
  }

  /**
   * The objects of the fixture. They cover the shapes the rule has to get right plus the two
   * kinds of ring centre: the L-shaped ring records one, a regular ring does not.
   */
  struct Fixture
  {
    std::vector<Area> areas;
    std::vector<Way>  ways;
  };

  Fixture MakeFixture(const TestTypes& types)
  {
    Fixture fixture;

    // A routable area: its ids are referenced once each.
    fixture.areas.push_back(MakeArea(types.routableAreaType,
                                     Nodes({1,2,3,4})));
    // A routable area that shares the id of its first node with the area above.
    fixture.areas.push_back(MakeArea(types.routableAreaType,
                                     Nodes({4,5,6})));
    // A routable area whose ring visits its first node twice.
    fixture.areas.push_back(MakeArea(types.routableAreaType,
                                     Nodes({7,7,8})));
    // An area that cannot route: it contributes nothing to the decision, but its serials
    // follow the global one.
    fixture.areas.push_back(MakeArea(types.plainAreaType,
                                     Nodes({9,10})));
    // A routable area whose pole of inaccessibility is not its bounding box centre.
    fixture.areas.push_back(MakeArea(types.routableAreaType,
                                     LShapedRing(0)));

    // A routable way that shares an id with the first area.
    fixture.ways.push_back(MakeWay(types.routableWayType,
                                   Nodes({2,11})));
    // A routable non-circular way that visits a node twice.
    fixture.ways.push_back(MakeWay(types.routableWayType,
                                   Nodes({12,12,13})));
    // A routable circular way that shares no id with another object.
    fixture.ways.push_back(MakeWay(types.routableWayType,
                                   Nodes({14,15,16,14})));
    // A way that cannot route.
    fixture.ways.push_back(MakeWay(types.plainWayType,
                                   Nodes({17,18})));

    return fixture;
  }

  TEST_CASE("The step provides the files the rule decides about, byte for byte as before")
  {
    TestTypes             types=MakeTypes();

    Fixture               fixture=MakeFixture(types);

    std::filesystem::path stepDirectory=MakeFixtureDirectory("step");
    std::filesystem::path referenceDirectory=MakeFixtureDirectory("reference");

    WriteAreas2(types,
                stepDirectory.string(),
                fixture.areas);
    WriteWayWay(types,
                stepDirectory.string(),
                fixture.ways);

    ImportParameter parameter;

    parameter.SetDestinationDirectory(stepDirectory.string());

    SilentProgress              progress;

    OptimizeAreaWayIdsGenerator generator;

    REQUIRE(generator.Import(types.typeConfig,
                             parameter,
                             progress));

    // The reference implementation of the same step, on the same input.
    ReferenceRule referenceRule;

    REQUIRE(ReferenceDecide(types,
                            stepDirectory.string(),
                            referenceRule));

    REQUIRE(ReferenceCopyAreas(types,
                               stepDirectory.string(),
                               referenceDirectory.string(),
                               referenceRule));
    REQUIRE(ReferenceCopyWays(types,
                              stepDirectory.string(),
                              referenceDirectory.string(),
                              referenceRule));

    RequireSameBytes(referenceDirectory/OptimizeAreaWayIdsGenerator::AREAS3_TMP,
                     stepDirectory/OptimizeAreaWayIdsGenerator::AREAS3_TMP);
    RequireSameBytes(referenceDirectory/OptimizeAreaWayIdsGenerator::WAYS_TMP,
                     stepDirectory/OptimizeAreaWayIdsGenerator::WAYS_TMP);
  }

  TEST_CASE("The serials of the provided files follow the rule")
  {
    TestTypes             types=MakeTypes();

    Fixture               fixture=MakeFixture(types);

    std::filesystem::path stepDirectory=MakeFixtureDirectory("decoded");

    WriteAreas2(types,
                stepDirectory.string(),
                fixture.areas);
    WriteWayWay(types,
                stepDirectory.string(),
                fixture.ways);

    ImportParameter parameter;

    parameter.SetDestinationDirectory(stepDirectory.string());

    SilentProgress              progress;

    OptimizeAreaWayIdsGenerator generator;

    REQUIRE(generator.Import(types.typeConfig,
                             parameter,
                             progress));

    // The areas: the id shared with a way keeps its serial, the ids of an object that cannot
    // route follow the decision of the routable objects and are cleared.
    {
      FileScanner scanner;

      scanner.Open(AppendFileToDir(stepDirectory.string(),
                                   OptimizeAreaWayIdsGenerator::AREAS3_TMP),
                   FileScanner::Sequential,
                   false);

      uint32_t areaCount=scanner.ReadUInt32();

      REQUIRE(areaCount==5);

      std::vector<std::vector<std::vector<Point>>> areas;

      for (uint32_t current=1; current<=areaCount; current++) {
        Area area;

        (void)scanner.ReadUInt8();
        (void)scanner.ReadInt64();

        area.Read(*types.typeConfig,
                  scanner);

        std::vector<std::vector<Point>> rings;

        rings.reserve(area.rings.size());

        for (const auto& ring : area.rings) {
          rings.push_back(ring.nodes);
        }

        areas.push_back(rings);
      }

      scanner.Close();

      // Area 0: nodes 1, 2, 3, 4. Node 2 is shared with the first way and node 4 with the second
      // area, so only those two keep their serial.
      REQUIRE_FALSE(Keeps(areas[0][0],0));
      REQUIRE(Keeps(areas[0][0],1));
      REQUIRE_FALSE(Keeps(areas[0][0],2));
      REQUIRE(Keeps(areas[0][0],3));

      // Area 1: nodes 4, 5, 6 - only the shared 4 keeps its serial.
      REQUIRE(Keeps(areas[1][0],0));
      REQUIRE_FALSE(Keeps(areas[1][0],1));
      REQUIRE_FALSE(Keeps(areas[1][0],2));

      // Area 2: nodes 7, 7, 8 - the repeated 7 counts once and is cleared.
      REQUIRE_FALSE(Keeps(areas[2][0],0));
      REQUIRE_FALSE(Keeps(areas[2][0],1));
      REQUIRE_FALSE(Keeps(areas[2][0],2));

      // Area 3 cannot route, so its ids are referenced by nothing and are cleared.
      REQUIRE_FALSE(Keeps(areas[3][0],0));
      REQUIRE_FALSE(Keeps(areas[3][0],1));

      // Area 4: the L-shaped ring, all of its ids are referenced once.
      for (const auto& node : areas[4][0]) {
        REQUIRE(node.GetSerial()==0);
      }
    }

    // The ways: the id shared with an area keeps its serial, the circular way keeps the id it
    // returns to, the way that cannot route follows the global decision.
    {
      FileScanner scanner;

      scanner.Open(AppendFileToDir(stepDirectory.string(),
                                   OptimizeAreaWayIdsGenerator::WAYS_TMP),
                   FileScanner::Sequential,
                   false);

      uint32_t wayCount=scanner.ReadUInt32();

      REQUIRE(wayCount==4);

      std::vector<std::vector<Point>> ways;

      ways.reserve(wayCount);

      for (uint32_t current=1; current<=wayCount; current++) {
        Way way;

        (void)scanner.ReadUInt8();
        (void)scanner.ReadInt64();

        way.Read(*types.typeConfig,
                 scanner);

        ways.push_back(way.nodes);
      }

      scanner.Close();

      // Way 0: nodes 2 and 11 - 2 is shared with the first area.
      REQUIRE(Keeps(ways[0],0));
      REQUIRE_FALSE(Keeps(ways[0],1));

      // Way 1: nodes 12, 12, 13 - the repeated 12 counts once.
      REQUIRE_FALSE(Keeps(ways[1],0));
      REQUIRE_FALSE(Keeps(ways[1],1));
      REQUIRE_FALSE(Keeps(ways[1],2));

      // Way 2 is circular: the id it returns to keeps its serial.
      REQUIRE(Keeps(ways[2],0));
      REQUIRE_FALSE(Keeps(ways[2],1));
      REQUIRE_FALSE(Keeps(ways[2],2));
      REQUIRE(Keeps(ways[2],3));

      // Way 3 cannot route.
      REQUIRE_FALSE(Keeps(ways[3],0));
      REQUIRE_FALSE(Keeps(ways[3],1));
    }
  }
}
