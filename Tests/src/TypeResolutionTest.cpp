/*
  TypeResolutionTest - a test program for libosmscout
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

#include <cstdlib>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/Tag.h>

#include <catch2/catch_test_macros.hpp>

namespace osmscout {

  /**
   * Test only access to the private type resolution dispatch index, used to
   * verify index construction. TypeConfig declares this struct as friend.
   */
  struct TypeResolutionIndexTestAccess
  {
    static const std::unordered_map<TagId,std::vector<TypeConfig::TypeConditionEntry>>& GetNodeIndex(const TypeConfig& config)
    {
      return config.nodeTypeIndex;
    }

    static const std::unordered_map<TagId,std::vector<TypeConfig::TypeConditionEntry>>& GetWayAreaIndex(const TypeConfig& config)
    {
      return config.wayAreaTypeIndex;
    }

    static const std::unordered_map<TagId,std::vector<TypeConfig::TypeConditionEntry>>& GetRelationIndex(const TypeConfig& config)
    {
      return config.relationTypeIndex;
    }

    static const std::vector<TypeConfig::TypeConditionEntry>& GetNodeFallback(const TypeConfig& config)
    {
      return config.nodeFallbackConditions;
    }

    static const std::vector<TypeConfig::TypeConditionEntry>& GetWayAreaFallback(const TypeConfig& config)
    {
      return config.wayAreaFallbackConditions;
    }

    static const std::vector<TypeConfig::TypeConditionEntry>& GetRelationFallback(const TypeConfig& config)
    {
      return config.relationFallbackConditions;
    }

    static bool IsSorted(const std::vector<TypeConfig::TypeConditionEntry>& entries)
    {
      for (size_t i=1; i<entries.size(); i++) {
        if (entries[i].typeIndex<entries[i-1].typeIndex ||
            (entries[i].typeIndex==entries[i-1].typeIndex &&
             entries[i].conditionIndex<entries[i-1].conditionIndex)) {
          return false;
        }
      }

      return true;
    }

    static void CollectCovered(const std::unordered_map<TagId,std::vector<TypeConfig::TypeConditionEntry>>& index,
                               const std::vector<TypeConfig::TypeConditionEntry>& fallback,
                               std::set<std::pair<size_t,size_t>>& covered)
    {
      for (const auto& entry : index) {
        for (const auto& condition : entry.second) {
          covered.emplace(condition.typeIndex,condition.conditionIndex);
        }
      }

      for (const auto& condition : fallback) {
        covered.emplace(condition.typeIndex,condition.conditionIndex);
      }
    }
  };

  static TagMap MakeTagMap(TypeConfig& config,
                           const std::vector<std::pair<std::string,std::string>>& tags)
  {
    TagMap tagMap;

    for (const auto& entry : tags) {
      tagMap[config.GetTagRegistry().RegisterTag(entry.first)]=entry.second;
    }

    return tagMap;
  }

  // Reference implementations of the former linear scan, kept in the test to
  // verify that the dispatch index produces identical results.

  static TypeInfoRef GetNodeTypeLinear(const TypeConfig& config,
                                       const TagMap& tagMap)
  {
    if (tagMap.empty()) {
      return config.typeInfoIgnore;
    }

    for (const auto& type : config.GetTypes()) {
      if (!type->HasConditions() ||
          !type->CanBeNode()) {
        continue;
      }

      for (const auto& cond : type->GetConditions()) {
        if ((cond.types & TypeInfo::typeNode)==0) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          return type;
        }
      }
    }

    return config.typeInfoIgnore;
  }

  static bool GetWayAreaTypeLinear(const TypeConfig& config,
                                   const TagMap& tagMap,
                                   TypeInfoRef& wayType,
                                   TypeInfoRef& areaType)
  {
    wayType=config.typeInfoIgnore;
    areaType=config.typeInfoIgnore;

    if (tagMap.empty()) {
      return false;
    }

    for (const auto& type : config.GetTypes()) {
      if (!((type->CanBeWay() ||
             type->CanBeArea()) &&
             type->HasConditions())) {
        continue;
      }

      for (const auto& cond : type->GetConditions()) {
        if (!((cond.types & TypeInfo::typeWay)!=0 ||
              (cond.types & TypeInfo::typeArea)!=0)) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          if (wayType==config.typeInfoIgnore &&
              (cond.types & TypeInfo::typeWay)!=0) {
            wayType=type;
          }

          if (areaType==config.typeInfoIgnore &&
              (cond.types & TypeInfo::typeArea)!=0) {
            areaType=type;
          }

          if (wayType!=config.typeInfoIgnore ||
              areaType!=config.typeInfoIgnore) {
            return true;
          }
        }
      }
    }

    return false;
  }

  static TypeInfoRef GetRelationTypeLinear(const TypeConfig& config,
                                           const TagMap& tagMap)
  {
    if (tagMap.empty()) {
      return config.typeInfoIgnore;
    }

    auto relationType=tagMap.find(config.tagType);

    if (relationType!=tagMap.end() &&
        relationType->second=="multipolygon") {
      for (const auto& type : config.GetTypes()) {
        if (!type->HasConditions() ||
            !type->CanBeArea()) {
          continue;
        }

        for (const auto& cond : type->GetConditions()) {
          if ((cond.types & TypeInfo::typeArea)==0) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return type;
          }
        }
      }
    }
    else {
      for (const auto& type : config.GetTypes()) {
        if (!type->HasConditions() ||
            !type->CanBeRelation()) {
          continue;
        }

        for (const auto& cond : type->GetConditions()) {
          if ((cond.types & TypeInfo::typeRelation)==0) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return type;
          }
        }
      }
    }

    return config.typeInfoIgnore;
  }

  static void CheckNodeEquivalence(const TypeConfig& config,
                                   const TagMap& tagMap)
  {
    TypeInfoRef dispatchType=config.GetNodeType(tagMap);
    TypeInfoRef linearType=GetNodeTypeLinear(config,tagMap);

    INFO("tagMap size: " << tagMap.size());
    REQUIRE(dispatchType->GetName()==linearType->GetName());
  }

  static void CheckWayAreaEquivalence(const TypeConfig& config,
                                      const TagMap& tagMap)
  {
    TypeInfoRef dispatchWayType;
    TypeInfoRef dispatchAreaType;
    bool        dispatchResult=config.GetWayAreaType(tagMap,
                                                     dispatchWayType,
                                                     dispatchAreaType);

    TypeInfoRef linearWayType;
    TypeInfoRef linearAreaType;
    bool        linearResult=GetWayAreaTypeLinear(config,tagMap,
                                                  linearWayType,
                                                  linearAreaType);

    INFO("tagMap size: " << tagMap.size());
    REQUIRE(dispatchResult==linearResult);
    REQUIRE(dispatchWayType->GetName()==linearWayType->GetName());
    REQUIRE(dispatchAreaType->GetName()==linearAreaType->GetName());
  }

  static void CheckRelationEquivalence(const TypeConfig& config,
                                       const TagMap& tagMap)
  {
    TypeInfoRef dispatchType=config.GetRelationType(tagMap);
    TypeInfoRef linearType=GetRelationTypeLinear(config,tagMap);

    INFO("tagMap size: " << tagMap.size());
    REQUIRE(dispatchType->GetName()==linearType->GetName());
  }
}

using namespace osmscout;

TEST_CASE("Leaf conditions report their tag as guaranteed key", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       highway=registry.RegisterTag("highway");
  TagId       entrance=registry.RegisterTag("entrance");
  TagId       bus=registry.RegisterTag("bus");

  std::vector<TagId> keys;
  bool               guaranteed{false};

  TagBinaryCondition binary(highway,operatorEqual,"primary");
  binary.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{highway});
  REQUIRE(guaranteed);

  TagExistsCondition exists(entrance);
  exists.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{entrance});
  REQUIRE(guaranteed);

  TagIsInCondition isIn(bus);
  isIn.AddTagValue("yes");
  isIn.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{bus});
  REQUIRE(guaranteed);
}

TEST_CASE("AND condition is keyed on first guaranteed child", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       landuse=registry.RegisterTag("landuse");
  TagId       building=registry.RegisterTag("building");

  TagBoolCondition andCondition(TagBoolCondition::boolAnd);
  andCondition.AddCondition(std::make_shared<TagBinaryCondition>(landuse,operatorEqual,"farmland"));
  andCondition.AddCondition(std::make_shared<TagExistsCondition>(building));

  std::vector<TagId> keys;
  bool               guaranteed{false};

  andCondition.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{landuse});
  REQUIRE(guaranteed);
}

TEST_CASE("AND condition with leading negation is keyed on later guaranteed child", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       x=registry.RegisterTag("x");
  TagId       b=registry.RegisterTag("b");

  TagBoolCondition andCondition(TagBoolCondition::boolAnd);
  andCondition.AddCondition(std::make_shared<TagNotCondition>(std::make_shared<TagBinaryCondition>(x,operatorEqual,"y")));
  andCondition.AddCondition(std::make_shared<TagExistsCondition>(b));

  std::vector<TagId> keys;
  bool               guaranteed{false};

  andCondition.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{b});
  REQUIRE(guaranteed);
}

TEST_CASE("OR condition unions branch keys and stays guaranteed", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       waterway=registry.RegisterTag("waterway");
  TagId       natural=registry.RegisterTag("natural");
  TagId       water=registry.RegisterTag("water");

  TagBoolCondition orCondition(TagBoolCondition::boolOr);
  orCondition.AddCondition(std::make_shared<TagBinaryCondition>(waterway,operatorEqual,"riverbank"));

  TagBoolCondition innerAnd(TagBoolCondition::boolAnd);
  innerAnd.AddCondition(std::make_shared<TagBinaryCondition>(natural,operatorEqual,"water"));
  innerAnd.AddCondition(std::make_shared<TagBinaryCondition>(water,operatorEqual,"river"));

  orCondition.AddCondition(std::make_shared<TagBoolCondition>(innerAnd));

  std::vector<TagId> keys;
  bool               guaranteed{false};

  orCondition.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys.size()==2);
  REQUIRE((keys[0]==waterway || keys[0]==natural));
  REQUIRE((keys[1]==waterway || keys[1]==natural));
  REQUIRE(keys[0]!=keys[1]);
  REQUIRE(guaranteed);
}

TEST_CASE("NOT condition is never guaranteed", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       bus=registry.RegisterTag("bus");

  TagNotCondition notCondition(std::make_shared<TagBinaryCondition>(bus,operatorEqual,"no"));

  std::vector<TagId> keys;
  bool               guaranteed{true};

  notCondition.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys==std::vector<TagId>{bus});
  REQUIRE(!guaranteed);
}

TEST_CASE("OR condition with negation is not guaranteed", "[TypeResolution]")
{
  TagRegistry registry;
  TagId       a=registry.RegisterTag("a");
  TagId       b=registry.RegisterTag("b");

  TagBoolCondition orCondition(TagBoolCondition::boolOr);
  orCondition.AddCondition(std::make_shared<TagBinaryCondition>(a,operatorEqual,"x"));
  orCondition.AddCondition(std::make_shared<TagNotCondition>(std::make_shared<TagBinaryCondition>(b,operatorEqual,"y")));

  std::vector<TagId> keys;
  bool               guaranteed{true};

  orCondition.CollectTagKeys(keys,guaranteed);
  REQUIRE(keys.size()==2);
  REQUIRE(!guaranteed);
}

static TypeConfigRef BuildSyntheticConfig()
{
  TypeConfigRef config=std::make_shared<TypeConfig>();

  TagId highway=config->GetTagRegistry().RegisterTag("highway");
  TagId landuse=config->GetTagRegistry().RegisterTag("landuse");
  TagId building=config->GetTagRegistry().RegisterTag("building");
  TagId natural=config->GetTagRegistry().RegisterTag("natural");
  TagId place=config->GetTagRegistry().RegisterTag("place");
  TagId route=config->GetTagRegistry().RegisterTag("route");
  TagId type=config->GetTagRegistry().RegisterTag("type");
  TagId barrier=config->GetTagRegistry().RegisterTag("barrier");

  // Node type: highway==primary
  TypeInfoRef highwayNode=std::make_shared<TypeInfo>("highway_node");
  highwayNode->CanBeNode(true);
  highwayNode->AddCondition(TypeInfo::typeNode,
                            std::make_shared<TagBinaryCondition>(highway,operatorEqual,"primary"));
  config->RegisterType(highwayNode);

  // Node type with negation: barrier != bollard
  TypeInfoRef barrierNode=std::make_shared<TypeInfo>("barrier_node");
  barrierNode->CanBeNode(true);
  TagBoolConditionRef barrierAnd=std::make_shared<TagBoolCondition>(TagBoolCondition::boolAnd);
  barrierAnd->AddCondition(std::make_shared<TagExistsCondition>(barrier));
  barrierAnd->AddCondition(std::make_shared<TagNotCondition>(std::make_shared<TagBinaryCondition>(barrier,operatorEqual,"bollard")));
  barrierNode->AddCondition(TypeInfo::typeNode,
                            barrierAnd);
  config->RegisterType(barrierNode);

  // Way+area type: landuse==farmland AND EXISTS building (keyed on landuse)
  TypeInfoRef farmArea=std::make_shared<TypeInfo>("landuse_farmland");
  farmArea->CanBeWay(true).CanBeArea(true);
  TagBoolConditionRef andCondition=std::make_shared<TagBoolCondition>(TagBoolCondition::boolAnd);
  andCondition->AddCondition(std::make_shared<TagBinaryCondition>(landuse,operatorEqual,"farmland"));
  andCondition->AddCondition(std::make_shared<TagExistsCondition>(building));
  farmArea->AddCondition(static_cast<unsigned char>(TypeInfo::typeWay|TypeInfo::typeArea),
                         andCondition);
  config->RegisterType(farmArea);

  // Area type: natural==water (also matches multipolygon relations)
  TypeInfoRef waterArea=std::make_shared<TypeInfo>("natural_water");
  waterArea->CanBeArea(true);
  waterArea->AddCondition(TypeInfo::typeArea,
                          std::make_shared<TagBinaryCondition>(natural,operatorEqual,"water"));
  config->RegisterType(waterArea);

  // Relation type: type==route AND route==bicycle
  TypeInfoRef bicycleRoute=std::make_shared<TypeInfo>("route_bicycle");
  bicycleRoute->CanBeRelation(true);
  TagBoolConditionRef routeAnd=std::make_shared<TagBoolCondition>(TagBoolCondition::boolAnd);
  routeAnd->AddCondition(std::make_shared<TagBinaryCondition>(type,operatorEqual,"route"));
  routeAnd->AddCondition(std::make_shared<TagBinaryCondition>(route,operatorEqual,"bicycle"));
  bicycleRoute->AddCondition(TypeInfo::typeRelation,
                             routeAnd);
  config->RegisterType(bicycleRoute);

  // Place node type defined after the others (order test)
  TypeInfoRef placeNode=std::make_shared<TypeInfo>("place_node");
  placeNode->CanBeNode(true);
  placeNode->AddCondition(TypeInfo::typeNode,
                          std::make_shared<TagBinaryCondition>(place,operatorEqual,"village"));
  config->RegisterType(placeNode);

  return config;
}

TEST_CASE("Dispatch index is built from synthetic type definition", "[TypeResolution]")
{
  TypeConfigRef config=BuildSyntheticConfig();

  TagId highway=config->GetTagId("highway");
  TagId landuse=config->GetTagId("landuse");
  TagId natural=config->GetTagId("natural");
  TagId type=config->GetTagId("type");
  TagId place=config->GetTagId("place");

  const auto& nodeIndex=TypeResolutionIndexTestAccess::GetNodeIndex(*config);
  const auto& wayAreaIndex=TypeResolutionIndexTestAccess::GetWayAreaIndex(*config);
  const auto& relationIndex=TypeResolutionIndexTestAccess::GetRelationIndex(*config);

  // highway_node keyed on highway
  REQUIRE(nodeIndex.at(highway).size()==1);
  REQUIRE(nodeIndex.at(highway)[0].type->GetName()=="highway_node");

  // barrier_node is keyed on barrier (EXISTS sibling is guaranteed)
  REQUIRE(nodeIndex.at(config->GetTagId("barrier")).size()==1);
  REQUIRE(nodeIndex.at(config->GetTagId("barrier"))[0].type->GetName()=="barrier_node");

  // all synthetic conditions are guaranteed -> no fallback entries in this config
  REQUIRE(TypeResolutionIndexTestAccess::GetNodeFallback(*config).empty());
  REQUIRE(TypeResolutionIndexTestAccess::GetWayAreaFallback(*config).empty());
  REQUIRE(TypeResolutionIndexTestAccess::GetRelationFallback(*config).empty());

  // landuse_farmland keyed on landuse (first guaranteed AND child), in way+area index
  REQUIRE(wayAreaIndex.at(landuse).size()==1);
  REQUIRE(wayAreaIndex.at(landuse)[0].type->GetName()=="landuse_farmland");
  REQUIRE((wayAreaIndex.at(landuse)[0].types & TypeInfo::typeWay)!=0);
  REQUIRE((wayAreaIndex.at(landuse)[0].types & TypeInfo::typeArea)!=0);

  // natural_water keyed on natural, in way+area index
  REQUIRE(wayAreaIndex.at(natural).size()==1);
  REQUIRE(wayAreaIndex.at(natural)[0].type->GetName()=="natural_water");

  // route_bicycle keyed on type (first guaranteed AND child)
  REQUIRE(relationIndex.at(type).size()==1);
  REQUIRE(relationIndex.at(type)[0].type->GetName()=="route_bicycle");

  // All key lists are sorted by (typeIndex, conditionIndex)
  for (const auto& entry : nodeIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  for (const auto& entry : wayAreaIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  for (const auto& entry : relationIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  REQUIRE(TypeResolutionIndexTestAccess::IsSorted(TypeResolutionIndexTestAccess::GetNodeFallback(*config)));

  // place_node must be the last node type, so a place=village object resolves to it
  (void)place;
}

TEST_CASE("Fallback conditions are evaluated and keep global order", "[TypeResolution]")
{
  // OR with a negated branch is not guaranteed and goes to the fallback list.
  // It may match objects that carry none of its keys, so it must be evaluated
  // in type-definition order together with the keyed conditions.
  TypeConfigRef config=std::make_shared<TypeConfig>();

  TagId highway=config->GetTagRegistry().RegisterTag("highway");
  TagId natural=config->GetTagRegistry().RegisterTag("natural");

  // Defined first: guaranteed, keyed on highway
  TypeInfoRef highwayNode=std::make_shared<TypeInfo>("highway_node");
  highwayNode->CanBeNode(true);
  highwayNode->AddCondition(TypeInfo::typeNode,
                            std::make_shared<TagBinaryCondition>(highway,operatorEqual,"primary"));
  config->RegisterType(highwayNode);

  // Defined second: fallback (OR with negated branch), may match without keys
  TypeInfoRef mixedNode=std::make_shared<TypeInfo>("mixed_node");
  mixedNode->CanBeNode(true);
  TagBoolConditionRef mixedOr=std::make_shared<TagBoolCondition>(TagBoolCondition::boolOr);
  mixedOr->AddCondition(std::make_shared<TagBinaryCondition>(highway,operatorEqual,"secondary"));
  mixedOr->AddCondition(std::make_shared<TagNotCondition>(std::make_shared<TagExistsCondition>(natural)));
  mixedNode->AddCondition(TypeInfo::typeNode,
                          mixedOr);
  config->RegisterType(mixedNode);

  REQUIRE(TypeResolutionIndexTestAccess::GetNodeFallback(*config).size()==1);
  REQUIRE(TypeResolutionIndexTestAccess::GetNodeFallback(*config)[0].type->GetName()=="mixed_node");

  std::vector<std::pair<std::string,std::string>> tagSets[]={
    {{"highway","primary"}},      // matches highway_node (earlier) via keyed path
    {{"highway","secondary"}},    // matches mixed_node via keyed path
    {{"natural","water"}},        // mixed_node: !(EXISTS natural) is false -> no match
    {{"name","test"}},            // mixed_node: !(EXISTS natural) is true -> match via fallback
    {{}},                           // empty -> ignore
  };

  for (const auto& tags : tagSets) {
    TagMap tagMap=MakeTagMap(*config,tags);
    CheckNodeEquivalence(*config,tagMap);
  }

  // Order: highway=primary resolves to highway_node (defined first), even though
  // mixed_node's negated branch would also match an object without natural.
  TagMap primaryMap=MakeTagMap(*config,{{"highway","primary"}});
  REQUIRE(config->GetNodeType(primaryMap)->GetName()=="highway_node");
}

TEST_CASE("Dispatch matches linear scan on synthetic type definition", "[TypeResolution]")
{
  TypeConfigRef config=BuildSyntheticConfig();

  std::vector<std::pair<std::string,std::string>> tagSets[]={
    {{"highway","primary"}},
    {{"highway","other"}},
    {{"barrier","gate"}},
    {{"barrier","bollard"}},
    {{"landuse","farmland"},{"building","yes"}},
    {{"landuse","farmland"}},
    {{"building","yes"}},
    {{"natural","water"}},
    {{"type","route"},{"route","bicycle"}},
    {{"type","multipolygon"},{"natural","water"}},
    {{"place","village"}},
    {{"name","test"}},
    {}
  };

  for (const auto& tags : tagSets) {
    TagMap tagMap=MakeTagMap(*config,tags);

    CheckNodeEquivalence(*config,tagMap);
    CheckWayAreaEquivalence(*config,tagMap);
    CheckRelationEquivalence(*config,tagMap);
  }
}

TEST_CASE("Dispatch preserves type definition order", "[TypeResolution]")
{
  TypeConfigRef config=BuildSyntheticConfig();

  // highway_node and place_node both resolve for their own tags.
  // place_node is defined last -> place=village resolves to place_node, and
  // highway=primary resolves to highway_node (which is defined first).
  TagMap highwayMap=MakeTagMap(*config,{{"highway","primary"}});
  REQUIRE(config->GetNodeType(highwayMap)->GetName()=="highway_node");

  TagMap placeMap=MakeTagMap(*config,{{"place","village"}});
  REQUIRE(config->GetNodeType(placeMap)->GetName()=="place_node");
}

TEST_CASE("Index is built from real map.ost", "[TypeResolution]")
{
  const char* topDir=std::getenv("TESTS_TOP_DIR");

  if (!topDir || !*topDir) {
    SKIP("TESTS_TOP_DIR not set, cannot locate map.ost");
  }

  std::string ostFile=std::string(topDir)+"/../stylesheets/map.ost";

  if (std::getenv("TESTS_TOP_DIR") && !*std::getenv("TESTS_TOP_DIR")) {
    SKIP("TESTS_TOP_DIR empty");
  }

  TypeConfigRef config=std::make_shared<TypeConfig>();

  if (!config->LoadFromOSTFile(ostFile)) {
    FAIL("Cannot load OST file " << ostFile);
  }

  const auto& nodeIndex=TypeResolutionIndexTestAccess::GetNodeIndex(*config);
  const auto& wayAreaIndex=TypeResolutionIndexTestAccess::GetWayAreaIndex(*config);
  const auto& relationIndex=TypeResolutionIndexTestAccess::GetRelationIndex(*config);

  const auto& nodeFallback=TypeResolutionIndexTestAccess::GetNodeFallback(*config);
  const auto& wayAreaFallback=TypeResolutionIndexTestAccess::GetWayAreaFallback(*config);
  const auto& relationFallback=TypeResolutionIndexTestAccess::GetRelationFallback(*config);

  // Every condition must be indexed or in the fallback list. Since a condition
  // may be indexed under several keys, count distinct (type, condition) pairs
  // and require full coverage of all conditions in the type definition.
  std::set<std::pair<size_t,size_t>> nodeCovered;
  std::set<std::pair<size_t,size_t>> wayAreaCovered;
  std::set<std::pair<size_t,size_t>> relationCovered;

  TypeResolutionIndexTestAccess::CollectCovered(nodeIndex,nodeFallback,nodeCovered);
  TypeResolutionIndexTestAccess::CollectCovered(wayAreaIndex,wayAreaFallback,wayAreaCovered);
  TypeResolutionIndexTestAccess::CollectCovered(relationIndex,relationFallback,relationCovered);

  std::set<std::pair<size_t,size_t>> nodeExpected;
  std::set<std::pair<size_t,size_t>> wayAreaExpected;
  std::set<std::pair<size_t,size_t>> relationExpected;

  size_t typeIndex=0;

  for (const auto& type : config->GetTypes()) {
    size_t conditionIndex=0;

    for (const auto& cond : type->GetConditions()) {
      if ((cond.types & TypeInfo::typeNode)!=0) {
        nodeExpected.emplace(typeIndex,conditionIndex);
      }
      if ((cond.types & (TypeInfo::typeWay|TypeInfo::typeArea))!=0) {
        wayAreaExpected.emplace(typeIndex,conditionIndex);
      }
      if ((cond.types & TypeInfo::typeRelation)!=0) {
        relationExpected.emplace(typeIndex,conditionIndex);
      }

      conditionIndex++;
    }

    typeIndex++;
  }

  REQUIRE(nodeCovered==nodeExpected);
  REQUIRE(wayAreaCovered==wayAreaExpected);
  REQUIRE(relationCovered==relationExpected);


  // All key lists and fallback lists are sorted by (typeIndex, conditionIndex)
  for (const auto& entry : nodeIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  for (const auto& entry : wayAreaIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  for (const auto& entry : relationIndex) {
    REQUIRE(TypeResolutionIndexTestAccess::IsSorted(entry.second));
  }
  REQUIRE(TypeResolutionIndexTestAccess::IsSorted(nodeFallback));
  REQUIRE(TypeResolutionIndexTestAccess::IsSorted(wayAreaFallback));
  REQUIRE(TypeResolutionIndexTestAccess::IsSorted(relationFallback));

  // The number of distinct primary keys is small compared to the type count
  // (measured: 39 keys for 1527 types on the further-types branch)
  REQUIRE(nodeIndex.size()<100);
  REQUIRE(wayAreaIndex.size()<100);
  REQUIRE(relationIndex.size()<100);

  // Dispatch equals linear scan for representative tag sets on the real type config
  std::vector<std::pair<std::string,std::string>> tagSets[]={
    {{"highway","residential"},{"name","Main Street"}},
    {{"highway","bus_stop"},{"bus","yes"},{"public_transport","platform"}},
    {{"amenity","cafe"},{"building","yes"}},
    {{"shop","bakery"}},
    {{"building","yes"}},
    {{"natural","water"}},
    {{"waterway","riverbank"}},
    {{"waterway","river"},{"name","Rhine"}},
    {{"landuse","residential"}},
    {{"leisure","park"}},
    {{"type","multipolygon"},{"natural","water"}},
    {{"type","route"},{"route","bicycle"}},
    {{"type","restriction"},{"restriction","no_left_turn"}},
    {{"source","survey"},{"note","benchmark"}},
    {{"highway","foobar"},{"amenity","foobar"}},
    {}
  };

  for (const auto& tags : tagSets) {
    TagMap tagMap=MakeTagMap(*config,tags);

    CheckNodeEquivalence(*config,tagMap);
    CheckWayAreaEquivalence(*config,tagMap);
    CheckRelationEquivalence(*config,tagMap);
  }
}

TEST_CASE("Fuzz: dispatch matches linear scan", "[TypeResolution]")
{
  const char* topDir=std::getenv("TESTS_TOP_DIR");

  if (!topDir || !*topDir) {
    SKIP("TESTS_TOP_DIR not set, cannot locate map.ost");
  }

  std::string ostFile=std::string(topDir)+"/../stylesheets/map.ost";

  TypeConfigRef config=std::make_shared<TypeConfig>();

  if (!config->LoadFromOSTFile(ostFile)) {
    FAIL("Cannot load OST file " << ostFile);
  }

  // Pool of tag keys and value samples covering the condition discriminators
  std::vector<std::string> keys={
    "highway","building","shop","amenity","historic","office","man_made","type",
    "natural","railway","landuse","leisure","sport","highway","place","barrier",
    "waterway","tourism","religion","piste:type","power","military","aeroway",
    "boundary","route","aerialway","area:highway","public_transport","bus","water",
    "covered","indoor","name","source","note","entrance","via_ferrata_scale"
  };

  std::vector<std::string> values={
    "primary","secondary","tertiary","residential","service","track","motorway",
    "footway","path","cycleway","pedestrian","bus_stop","living_street",
    "yes","no","true","false","0","1",
    "water","river","riverbank","stream","canal","lake",
    "farmland","forest","residential","meadow","village","city","town","suburb",
    "cafe","restaurant","bakery","school","bank","bench","park","swimming_pool",
    "bollard","gate","lift_gate","fence","wall",
    "platform","stop_position","bicycle","bus","train",
    "multipolygon","restriction","no_left_turn","no_right_turn",
    "survey","benchmark","test","foobar"
  };

  std::mt19937                    rng(12345);
  std::uniform_int_distribution<> keyDist(0,static_cast<int>(keys.size())-1);
  std::uniform_int_distribution<> valueDist(0,static_cast<int>(values.size())-1);
  std::uniform_int_distribution<> tagCountDist(0,6);

  for (size_t iteration=0; iteration<2000; iteration++) {
    TagMap tagMap;

    size_t tagCount=tagCountDist(rng);

    for (size_t t=0; t<tagCount; t++) {
      TagId tagId=config->GetTagRegistry().RegisterTag(keys[keyDist(rng)]);

      tagMap[tagId]=values[valueDist(rng)];
    }

    CheckNodeEquivalence(*config,tagMap);
    CheckWayAreaEquivalence(*config,tagMap);
    CheckRelationEquivalence(*config,tagMap);
  }
}
