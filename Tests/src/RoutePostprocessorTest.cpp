/*
  RoutePostprocessor - a test program for libosmscout
  Copyright (C) 2024  Lukas Karas

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

#include <cstdio>
#include <string>
#include <filesystem>

#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/log/Logger.h>

#include <catch2/catch_test_macros.hpp>

using namespace std;
using namespace osmscout;

struct TmpFile
{
  std::filesystem::path path;

  TmpFile(const std::string &baseName, const std::string &suffix)
  {
    for (int i=0;; i++) {
      path = filesystem::temp_directory_path() / (baseName + std::to_string(i) + suffix);
      if (!std::filesystem::exists(path)) { // there is possible race condition between this check and file creation...
        break;
      }
    }
  };

  TmpFile(const TmpFile&) = delete;
  TmpFile(TmpFile&&) = delete;
  TmpFile& operator=(const TmpFile&) = delete;
  TmpFile& operator=(TmpFile&&) = delete;

  ~TmpFile()
  {
    if (filesystem::exists(path)) {
      filesystem::remove(path);
    }
  }
};

using TmpFileRef=std::shared_ptr<TmpFile>;


class MockDatabase {
private:
  TypeConfigRef typeConfig;
  TmpFileRef wayFile;
  FileScanner wayScanner;

public:
  TypeInfoRef highwayType;
  TypeInfoRef roundaboutType;
  TypeInfoRef motorwayType;
  TypeInfoRef motorwayLinkType;

public:
  MockDatabase(TypeConfigRef typeConfig,
               const TmpFileRef &wayFile,
               const TypeInfoRef &highwayType,
               const TypeInfoRef &roundaboutType,
               const TypeInfoRef &motorwayType,
               const TypeInfoRef &motorwayLinkType):
    typeConfig(typeConfig),
    wayFile(wayFile),
    highwayType(highwayType),
    roundaboutType(roundaboutType),
    motorwayType(motorwayType),
    motorwayLinkType(motorwayLinkType)
  {
    wayScanner.Open(wayFile->path.string(), FileScanner::Mode::FastRandom, true);
  }

  ~MockDatabase()
  {
    wayScanner.Close();
  };

  osmscout::WayRef GetWay(const osmscout::DBFileOffset &offset)
  {
    assert(offset.database==0);
    auto way=std::make_shared<Way>();
    wayScanner.SetPos(offset.offset);
    way->Read(*typeConfig, wayScanner);
    return way;
  }

  TypeConfigRef GetTypeConfig() {
    return typeConfig;
  }
};

using MockDatabaseRef = std::shared_ptr<MockDatabase>;
using FeatureSetupFn = std::function<void(AccessFeatureValue*, LanesFeatureValue*, NameFeatureValue*)>;

class MockDatabaseBuilder {
private:
  TypeConfigRef typeConfig=std::make_shared<TypeConfig>();
  FileWriter writer;
  TmpFileRef wayFile=std::make_shared<TmpFile>("test-ways", ".dat");
  osmscout::FeatureRef laneFeature;
  osmscout::FeatureRef accessFeature;
  osmscout::FeatureRef nameFeature;

  TypeInfoRef highwayType;
  TypeInfoRef roundaboutType;
  TypeInfoRef motorwayType;
  TypeInfoRef motorwayLinkType;

  // needs to correspond to feature registration order
  static constexpr size_t laneFeatureIndex=0;
  static constexpr size_t accessFeatureIndex=1;
  static constexpr size_t nameFeatureIndex=2;

public:
  MockDatabaseBuilder()
  {
    laneFeature = typeConfig->GetFeature(LanesFeature::NAME);
    assert(laneFeature);
    accessFeature = typeConfig->GetFeature(AccessFeature::NAME);
    assert(accessFeature);
    nameFeature = typeConfig->GetFeature(NameFeature::NAME);
    assert(nameFeature);

    highwayType = RegisterHighwayType("highway");
    roundaboutType = RegisterHighwayType("roundabout");
    motorwayType = RegisterHighwayType("motorway");
    motorwayLinkType = RegisterHighwayType("motorway_link");

    writer.Open(wayFile->path.string());
  }

  ~MockDatabaseBuilder()
  {
    if (writer.IsOpen()) {
      writer.Close();
    }
  }

  TypeInfoRef RegisterHighwayType(const std::string &name)
  {
    TypeInfoRef type=std::make_shared<TypeInfo>(name);
    // type->SetInternal()
    type->CanBeWay(true)
      .CanRouteCar(true)
      .AddFeature(laneFeature)
      .AddFeature(accessFeature)
      .AddFeature(nameFeature);

    typeConfig->RegisterType(type);
    return type;
  }

  ObjectFileRef AddHighway(const std::vector<GeoCoord> &coords, FeatureSetupFn setupFeatures)
  {
    return AddWay(highwayType, coords, setupFeatures);
  }

  ObjectFileRef AddRoundabout(const std::vector<GeoCoord> &coords, FeatureSetupFn setupFeatures)
  {
    return AddWay(roundaboutType, coords, setupFeatures);
  }

  ObjectFileRef AddMotorway(const std::vector<GeoCoord> &coords, FeatureSetupFn setupFeatures)
  {
    return AddWay(motorwayType, coords, setupFeatures);
  }

  ObjectFileRef AddMotorwayLink(const std::vector<GeoCoord> &coords, FeatureSetupFn setupFeatures)
  {
    return AddWay(motorwayLinkType, coords, setupFeatures);
  }

  ObjectFileRef AddWay(const TypeInfoRef &type, const std::vector<GeoCoord> &coords, FeatureSetupFn setupFeatures)
  {
    Way way;

    FeatureValueBuffer featureValueBuffer;
    featureValueBuffer.SetType(type);

    setupFeatures(static_cast<AccessFeatureValue *>(featureValueBuffer.AllocateValue(accessFeatureIndex)),
                  static_cast<LanesFeatureValue *>(featureValueBuffer.AllocateValue(laneFeatureIndex)),
                  static_cast<NameFeatureValue *>(featureValueBuffer.AllocateValue(nameFeatureIndex)));

    way.SetFeatures(featureValueBuffer);

    for (const GeoCoord &coord : coords) {
      way.nodes.push_back(Point(0, coord));
    }
    ObjectFileRef wayRef(writer.GetPos(), RefType::refWay);
    way.Write(*typeConfig, writer);
    return wayRef;
  }

  MockDatabaseRef Build()
  {
    writer.Close();
    return std::make_shared<MockDatabase>(typeConfig, wayFile, highwayType, roundaboutType, motorwayType, motorwayLinkType);
  }
};

class MockContext: public osmscout::PostprocessorContext
{
private:
  MockDatabaseRef database;
  LanesFeatureValueReader lanesReader;
  AccessFeatureValueReader accessReader;
  NameFeatureValueReader nameReader;

public:
  explicit MockContext(MockDatabaseRef database):
    database(database),
    lanesReader(*database->GetTypeConfig()),
    accessReader(*database->GetTypeConfig()),
    nameReader(*database->GetTypeConfig())
  {}

  osmscout::AreaRef GetArea([[maybe_unused]] const osmscout::DBFileOffset &offset) const override
  {
    assert(false);
    return osmscout::AreaRef();
  }

  osmscout::WayRef GetWay(const osmscout::DBFileOffset &offset) const override
  {
    return database->GetWay(offset);
  }

  osmscout::NodeRef GetNode([[maybe_unused]] const osmscout::DBFileOffset &offset) const override
  {
    assert(false);
    return osmscout::NodeRef();
  }

  const LanesFeatureValueReader& GetLaneReader([[maybe_unused]] const DatabaseId &dbId) const override
  {
    assert(dbId==0);
    return lanesReader;
  }

  const AccessFeatureValueReader& GetAccessReader([[maybe_unused]] const DatabaseId &dbId) const override
  {
    assert(dbId==0);
    return accessReader;
  }


  osmscout::Duration
  GetTime([[maybe_unused]] osmscout::DatabaseId dbId, [[maybe_unused]] const osmscout::Area &area, [[maybe_unused]] const osmscout::Distance &deltaDistance) const override
  {
    assert(false);
    return osmscout::Duration();
  }

  osmscout::Duration
  GetTime([[maybe_unused]] osmscout::DatabaseId dbId, [[maybe_unused]] const osmscout::Way &way, [[maybe_unused]] const osmscout::Distance &deltaDistance) const override
  {
    assert(false);
    return osmscout::Duration();
  }

  osmscout::RouteDescription::NameDescriptionRef
  GetNameDescription([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return osmscout::RouteDescription::NameDescriptionRef();
  }

  osmscout::RouteDescription::NameDescriptionRef
  GetNameDescription([[maybe_unused]] osmscout::DatabaseId dbId,
                     [[maybe_unused]] const osmscout::ObjectFileRef &object) const override
  {
    assert(false);
    return osmscout::RouteDescription::NameDescriptionRef();
  }

  osmscout::RouteDescription::NameDescriptionRef
  GetNameDescription([[maybe_unused]] osmscout::DatabaseId dbId,
                     [[maybe_unused]] const osmscout::Node &node) const override
  {
    assert(false);
    return osmscout::RouteDescription::NameDescriptionRef();
  }

  osmscout::RouteDescription::NameDescriptionRef
  GetNameDescription([[maybe_unused]] osmscout::DatabaseId dbId,
                     [[maybe_unused]] const osmscout::Area &area) const override
  {
    assert(false);
    return osmscout::RouteDescription::NameDescriptionRef();
  }

  osmscout::RouteDescription::NameDescriptionRef
  GetNameDescription([[maybe_unused]] osmscout::DatabaseId dbId,
                     [[maybe_unused]] const osmscout::Way &way) const override
  {
    NameFeatureValue *nameValue=nameReader.GetValue(way.GetFeatureValueBuffer());
    if (nameValue) {
      return std::make_shared<RouteDescription::NameDescription>(nameValue->GetName(), "");
    }

    return std::make_shared<RouteDescription::NameDescription>("", "");
  }

  bool IsMotorwayLink(const osmscout::RouteDescription::Node &node) const override
  {
    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return database->motorwayLinkType == way->GetType();
    }
    return false;
  }

  bool IsMotorway(const osmscout::RouteDescription::Node &node) const override
  {
    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return database->motorwayType == way->GetType();
    }
    return false;
  }

  bool IsMiniRoundabout([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    return false;
  }

  bool IsClockwise([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsRoundabout([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return database->roundaboutType == way->GetType();
    }
    return false;
  }

  bool IsBridge([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    return false;
  }

  osmscout::NodeRef GetJunctionNode([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return osmscout::NodeRef();
  }

  osmscout::RouteDescription::DestinationDescriptionRef
  GetDestination([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return osmscout::RouteDescription::DestinationDescriptionRef();
  }

  uint8_t GetMaxSpeed([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return 0;
  }

  size_t GetNodeIndex([[maybe_unused]] const osmscout::RouteDescription::Node &node,
                      [[maybe_unused]] osmscout::Id nodeId) const override
  {
    assert(false);
    return 0;
  }

  bool CanUseBackward(const osmscout::DatabaseId &dbId, osmscout::Id fromNodeId,
                      const osmscout::ObjectFileRef &object) const override
  {
    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      if (fromNodeIndex==0) {
        return false;
      }

      AccessFeatureValue *accessValue=accessReader.GetValue(way->GetFeatureValueBuffer());
      if (!accessValue){
        return true;
      }
      return accessValue->CanRouteCarBackward();
    }

    assert(false);
    return false;
  }

  bool CanUseForward(const osmscout::DatabaseId &dbId, osmscout::Id fromNodeId,
                     const osmscout::ObjectFileRef &object) const override
  {
    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,
                                     object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      if (fromNodeIndex==way->nodes.size()-1) {
        return false;
      }
      AccessFeatureValue *accessValue=accessReader.GetValue(way->GetFeatureValueBuffer());
      if (!accessValue){
        return true;
      }
      return accessValue->CanRouteCarForward();
    }
    assert(false);
    return false;
  }

  bool IsBackwardPath([[maybe_unused]] const osmscout::ObjectFileRef &object,
                      [[maybe_unused]] size_t fromNodeIndex,
                      [[maybe_unused]] size_t toNodeIndex) const override
  {
    assert(false);
    return false;
  }

  bool IsForwardPath([[maybe_unused]] const osmscout::ObjectFileRef &object,
                     [[maybe_unused]] size_t fromNodeIndex,
                     [[maybe_unused]] size_t toNodeIndex) const override
  {
    assert(false);
    return false;
  }

  bool IsNodeStartOrEndOfObject([[maybe_unused]] const osmscout::RouteDescription::Node &node,
                                [[maybe_unused]] const osmscout::ObjectFileRef &object) const override
  {
    assert(false);
    return false;
  }

  osmscout::GeoCoord GetCoordinates(const osmscout::RouteDescription::Node &node,
                                    size_t nodeIndex) const override
  {
    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return way->GetCoord(nodeIndex);
    }
    assert(false);
    return osmscout::GeoCoord();
  }

  vector<osmscout::DatabaseRef> GetDatabases() const override
  {
    assert(false);
    return vector<osmscout::DatabaseRef>();
  }
};

void Postprocess(RouteDescription &description, MockContext &context)
{
  RoutePostprocessor::LanesPostprocessor().Process(context, description);
  RoutePostprocessor::SuggestedLanesPostprocessor().Process(context, description);

  RoutePostprocessor::WayNamePostprocessor().Process(context, description);
  RoutePostprocessor::DirectionPostprocessor().Process(context, description);
  RoutePostprocessor::InstructionPostprocessor().Process(context, description);
}

TEST_CASE("Describe simple junction")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/#map=19/50.09267/14.53044
  MockDatabaseBuilder databaseBuilder;
  // street Českobrodská to east https://www.openstreetmap.org/way/936270761
  // with two lanes (street was split to separate oneway for east and west direction at the summer 2024)
  ObjectFileRef way1Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.09294, 14.52899), GeoCoord(50.09282, 14.52976)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Českobrodská");
    });

  // continuation of Českobrodská https://www.openstreetmap.org/way/62143601
  // single lane
  ObjectFileRef way2Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.09282, 14.52976), GeoCoord(50.09263, 14.53118)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(1, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Českobrodská");
    });

  // link between Českobrodská a Průmyslová https://www.openstreetmap.org/way/936270757
  // single lane
  ObjectFileRef way3Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.09282, 14.52976), GeoCoord(50.09261, 14.53093)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue*){
      lanes->SetLanes(1, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });


  MockContext context(databaseBuilder.Build());

  {
    RouteDescription description;
    // route is going in direction to Průmyslová
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way3Ref, 1);
    description.AddNode(0, 1, {way3Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));

    // should suggest the right lane
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Unknown);

    // direction change is minor, it is evaluated as straight on
    // it is the problem of the test data, it doesn't include longer segment of target way
    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // no turn description as there is no significant direction change
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC) == nullptr);
  }

  {
    RouteDescription description;

    // route continue to the east
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way2Ref, 1);
    description.AddNode(0, 1, {way2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest left lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Unknown);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // no explicit turn when continue on the same way
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC) == nullptr);
  }
}


TEST_CASE("Describe simple junction with lane turns")
{
  using namespace osmscout;

  // same street as before, next exit (February 2025)
  // https://www.openstreetmap.org/#map=19/50.092009/14.534488&layers=N
  MockDatabaseBuilder databaseBuilder;
  // street Českobrodská to east https://www.openstreetmap.org/way/1327814207
  // with three lanes forward, two backward
  ObjectFileRef way1Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.092173, 14.533681), GeoCoord(50.092102, 14.534022)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(3, 2);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through, LaneTurn::Right}, {LaneTurn::None, LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
      name->SetName("Českobrodská");
    });

  // continuation of Českobrodská https://www.openstreetmap.org/way/1327814206
  // left and through lanes continue...
  ObjectFileRef way2Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.092102, 14.534022), GeoCoord(50.092038, 14.534400)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 2);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through}, {LaneTurn::None, LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
      name->SetName("Českobrodská");
    });

  // link between Českobrodská a Průmyslová https://www.openstreetmap.org/way/4779018
  // single lane
  ObjectFileRef way3Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.092102, 14.534022), GeoCoord(50.091858, 14.534306)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue*){
      lanes->SetLanes(1, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });


  MockContext context(databaseBuilder.Build());

  {
    RouteDescription description;

    // route is going in direction to Průmyslová
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way3Ref, 1);
    description.AddNode(0, 1, {way3Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));

    // should suggest the right lane
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 2);
    REQUIRE(suggestedLanes->GetTo() == 2);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Right);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::slightlyRight);
    // explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC));
  }

  {
    RouteDescription description;

    // route continue to the east
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way2Ref, 1);
    description.AddNode(0, 1, {way2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest the central lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Unknown);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // no explicit turn when continue on the same way
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC) == nullptr);
  }
}


TEST_CASE("Describe slightly complex junction")
{
  using namespace osmscout;

  ::osmscout::log.Debug(true);

  // https://www.openstreetmap.org/#map=19/50.077838/14.511968
  MockDatabaseBuilder databaseBuilder;
  // street Dřevčická to south https://www.openstreetmap.org/way/1308678203
  // with three lanes
  ObjectFileRef drevcicka1Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.078024, 14.511979), GeoCoord(50.077919, 14.511966)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 1);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Right}, {LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
      name->SetName("Dřevčická");
    });

  // continuation of Dřevčická street to south https://www.openstreetmap.org/way/501228496
  // with two lanes
  ObjectFileRef drevcicka2Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.077919, 14.511966), GeoCoord(50.077766, 14.511963)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(1, 1);
      lanes->SetTurnLanes({LaneTurn::Left}, {LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
      name->SetName("Dřevčická");
    });

  // north lanes of Černokostelecká, from east to west https://www.openstreetmap.org/way/22823101
  ObjectFileRef northCernokostelecka1Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.077845, 14.513782), GeoCoord(50.077919, 14.511966)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through_Right}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Černokostelecká");
    });

  // https://www.openstreetmap.org/way/1094576469
  ObjectFileRef northCernokostelecka2Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.077919, 14.511966), GeoCoord(50.077937, 14.510528)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Černokostelecká");
    });

  // south lanes of Černokostelecká, from west to east https://www.openstreetmap.org/way/1094576467
  ObjectFileRef southCernokostelecka1Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.077776, 14.510397), GeoCoord(50.077766, 14.511963)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Černokostelecká");
    });

  // https://www.openstreetmap.org/way/1094576468
  ObjectFileRef southCernokostelecka2Ref=databaseBuilder.AddHighway(
    {GeoCoord(50.077766, 14.511963), GeoCoord(50.077685,14.513725)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
      name->SetName("Černokostelecká");
    });

  MockContext context(databaseBuilder.Build());

  { // route is going from west to east on Cernokostelecka
    RouteDescription description;

    description.AddNode(0, 0, {southCernokostelecka1Ref}, southCernokostelecka1Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, southCernokostelecka2Ref, 1);
    description.AddNode(0, 1, {southCernokostelecka2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Through);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC) == nullptr);
  }

  { // route is going from west on Cernokostelecka to north on Drevcicka
    RouteDescription description;

    description.AddNode(0, 0, {southCernokostelecka1Ref}, southCernokostelecka1Ref, 1);
    description.AddNode(0, 1, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, drevcicka2Ref, 0);
    description.AddNode(0, 1, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka1Ref, 0);
    description.AddNode(0, 1, {drevcicka1Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest left lane, left turn
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Left);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::left);
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC));
  }

  { // route is going from the east to west on Cernokostelecka
    RouteDescription description;

    description.AddNode(0, 0, {northCernokostelecka1Ref}, northCernokostelecka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, northCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // both lanes are usable, we don't need suggestion as going through, but let's provide it
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Through);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC) == nullptr);
  }

  { // route is going from the east on Cernokostelecka to north on Drevcicka
    RouteDescription description;

    description.AddNode(0, 0, {northCernokostelecka1Ref}, northCernokostelecka1Ref, 1);
    description.AddNode(0, 1, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka1Ref, 0);
    description.AddNode(0, 0, {drevcicka1Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest right lane to the right
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Right);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::right);
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC));
  }

  { // route is going from the north from Drevcicka to the west on Cernokostelecka
    RouteDescription description;

    description.AddNode(0, 0, {drevcicka1Ref}, drevcicka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, northCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Right);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::right);
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC));
  }

  { // route is going from the north from Drevcicka to the east on Cernokostelecka
    RouteDescription description;

    description.AddNode(0, 0, {drevcicka1Ref}, drevcicka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka2Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, southCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka2Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest left lane to the left
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Left);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // continue straight, no explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC)==nullptr);

    ++nodeIt;
    directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::left);
    // there is just one possible exit from the way, it is expected that there is no need for explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC)==nullptr);
    // TODO: there should be explicit turn, probably on the second node as the lane turn indicates
  }
}

TEST_CASE("Describe complex city junction: Průmyslová, Černokostecká")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/#map=18/50.071261/14.536248

  MockDatabaseBuilder databaseBuilder;

  // west lanes of "Prumyslova" highway, directing to south https://www.openstreetmap.org/way/313479831
  // with four lanes
  ObjectFileRef prumyslovaWest1Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071767, 14.537331), GeoCoord(50.071368, 14.537289)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // continuation, the middle of the junction with four lanes... https://www.openstreetmap.org/way/1333169461
  ObjectFileRef prumyslovaWest2Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071368, 14.537289), GeoCoord(50.071256, 14.537254)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // continuation after junction, with two lanes... https://www.openstreetmap.org/way/896731161
  ObjectFileRef prumyslovaWest3Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071256, 14.537254), GeoCoord(50.071030, 14.537224)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // east lanes of "Prumyslova" highway, directing to north https://www.openstreetmap.org/way/1024991959
  // with three lanes
  ObjectFileRef prumyslovaEast1Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.070965, 14.537447), GeoCoord(50.071228, 14.537474)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // continuation, the middle of the junction with three lanes https://www.openstreetmap.org/way/1333169460
  ObjectFileRef prumyslovaEast2Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071228, 14.537474), GeoCoord(50.071347, 14.537514)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // continuation, after junction with two lanes https://www.openstreetmap.org/way/896731147
  ObjectFileRef prumyslovaEast3Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071347, 14.537514), GeoCoord(50.071669, 14.537536)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Prumyslova");
    });

  // north lanes of "Cernokostelecka", directing to west https://www.openstreetmap.org/way/219625343
  ObjectFileRef cernokosteleckaNorth1Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071328, 14.538123), GeoCoord(50.071347, 14.537514)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  // continuation, middle of the junction https://www.openstreetmap.org/way/1333169462
  ObjectFileRef cernokosteleckaNorth2Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071347, 14.537514), GeoCoord(50.071368, 14.537289)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  // continuation after the junction https://www.openstreetmap.org/way/896731169
  ObjectFileRef cernokosteleckaNorth3Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071368, 14.537289), GeoCoord(50.071416, 14.536884)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  // south lanes of "Cernokostelecka", directing to east https://www.openstreetmap.org/way/360156805
  ObjectFileRef cernokosteleckaSouth1Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071309, 14.536822), GeoCoord(50.071256, 14.537254)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  // continuation, middle of the junction https://www.openstreetmap.org/way/1333169463
  ObjectFileRef cernokosteleckaSouth2Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071256, 14.537254), GeoCoord(50.071228, 14.537474)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(4, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Left, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  // continuation, after the junction https://www.openstreetmap.org/way/896731153
  ObjectFileRef cernokosteleckaSouth3Ref=databaseBuilder.AddMotorway(
    {GeoCoord(50.071228, 14.537474), GeoCoord(50.071211, 14.537850)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Cernokostelecka");
    });

  MockContext context(databaseBuilder.Build());

  { // route is going from the north from Prumyslova to the east Cernokostelecka
    RouteDescription description;

    description.AddNode(0, 0, {prumyslovaWest1Ref}, prumyslovaWest1Ref, 1);
    description.AddNode(0, 0, {prumyslovaWest1Ref, prumyslovaWest2Ref, cernokosteleckaNorth2Ref, cernokosteleckaNorth3Ref}, prumyslovaWest2Ref, 1);
    description.AddNode(0, 0, {prumyslovaWest2Ref, prumyslovaWest3Ref, cernokosteleckaSouth1Ref, cernokosteleckaSouth2Ref}, cernokosteleckaSouth2Ref, 1);
    description.AddNode(0, 0, {cernokosteleckaSouth2Ref, cernokosteleckaSouth3Ref, prumyslovaEast1Ref, prumyslovaEast2Ref}, cernokosteleckaSouth3Ref, 1);
    description.AddNode(0, 0, {cernokosteleckaSouth3Ref}, ObjectFileRef(), 1);

    Postprocess(description, context);

    // should suggest two lanes to the left
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    // TODO: improve lane evaluation on similar junctions
    // REQUIRE(suggestedLanes->GetFrom() == 0);
    // REQUIRE(suggestedLanes->GetTo() == 1);
    // REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Left);
  }
}

TEST_CASE("Describe complex city junction: Na Strži, Na Pankráci")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/#map=19/50.050041/14.442011

  MockDatabaseBuilder databaseBuilder;

  // north lanes of Na Strzi: https://www.openstreetmap.org/way/4647670
  ObjectFileRef naStrziNorth1Ref = databaseBuilder.AddMotorway(
    {GeoCoord(50.050415, 14.441638), GeoCoord(50.050149, 14.441129)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue *name) {
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through, LaneTurn::Through_Right}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Na Strzi");
    });

  // continuation: https://www.openstreetmap.org/way/218366354
  ObjectFileRef naStrziNorth2Ref = databaseBuilder.AddMotorway(
    {GeoCoord(50.050149, 14.441129), GeoCoord(50.050026, 14.440976)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue *name) {
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through, LaneTurn::Through_Right}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Na Strzi");
    });

  // east lanes of no name: https://www.openstreetmap.org/way/4415195
  ObjectFileRef noNameEast1Ref = databaseBuilder.AddMotorway(
    {GeoCoord(50.050021, 14.441265), GeoCoord(50.050149, 14.441129)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue *) {
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // east lanes of Na Pankraci: https://www.openstreetmap.org/way/1340371643
  ObjectFileRef naPankraciEast2Ref = databaseBuilder.AddMotorway(
    {GeoCoord(50.050149, 14.441129), GeoCoord(50.050434, 14.440868)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue *name) {
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::None, LaneTurn::None}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("Na Pankraci");
    });

  MockContext context(databaseBuilder.Build());

  { // route is going from the east to the north
    RouteDescription description;

    description.AddNode(0, 0, {naStrziNorth1Ref}, naStrziNorth1Ref, 1);
    description.AddNode(0, 0, {naPankraciEast2Ref, naStrziNorth1Ref, naStrziNorth2Ref, noNameEast1Ref}, naPankraciEast2Ref, 1);
    description.AddNode(0, 0, {naPankraciEast2Ref}, ObjectFileRef(), 1);

    Postprocess(description, context);

    // should suggest right lane to the right
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 2);
    REQUIRE(suggestedLanes->GetTo() == 2);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Right);
  }
}

TEST_CASE("Describe A3/A4 highway split")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/query?lat=47.341840&lon=8.454964#map=19/47.341780/8.453880
  MockDatabaseBuilder databaseBuilder;
  // highway A3/A4 https://www.openstreetmap.org/way/1304846969
  // with three lanes
  ObjectFileRef a3a4Ref=databaseBuilder.AddMotorway(
    {GeoCoord(47.341814, 8.453848), GeoCoord(47.341844, 8.454816)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("A3/A4");
    });

  // highway A3 https://www.openstreetmap.org/way/38698216
  // with two lanes
  ObjectFileRef a3Ref=databaseBuilder.AddMotorway(
    {GeoCoord(47.341844, 8.454816), GeoCoord(47.341923, 8.456367)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("A3");
    });

  // highway A4 https://www.openstreetmap.org/way/1205837033
  // with two lanes
  ObjectFileRef a4Ref=databaseBuilder.AddMotorway(
    {GeoCoord(47.341844, 8.454816), GeoCoord(47.341756, 8.455870)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes, NameFeatureValue* name){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
      name->SetName("A4");
    });

  MockContext context(databaseBuilder.Build());

  {
    RouteDescription description;

    // route continue to A4
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a4Ref, 1);
    description.AddNode(0, 1, {a4Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest right lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 2);
    REQUIRE(suggestedLanes->GetTo() == 2);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::SlightRight);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // continue straight, no explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC)==nullptr);
    // TODO: there should be explicit right turn (MotorwayChange) as the right lane was suggested on previous node and motorway was changed
  }

  {
    RouteDescription description;

    // route continue to A3
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a3Ref, 1);
    description.AddNode(0, 1, {a3Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest two left through lanes
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 1);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Through);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // continue straight, no explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC)==nullptr);
  }
}

TEST_CASE("Describe A3/A4 highway near Zurich")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/way/116756494#map=18/47.407778/8.424867
  MockDatabaseBuilder databaseBuilder;

  // highway A3/A4 https://www.openstreetmap.org/way/116756494
  // with three lanes
  ObjectFileRef a3a4Ref = databaseBuilder.AddMotorway(
    {GeoCoord(47.407328, 8.423713), GeoCoord(47.407877, 8.424234)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue*) {
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::SlightLeft, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway (motorway link) A3 https://www.openstreetmap.org/way/26834628
  // with one lane
  ObjectFileRef a3Ref = databaseBuilder.AddMotorwayLink(
    {GeoCoord(47.407877, 8.424234), GeoCoord(47.409503, 8.425961)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue*) {
      lanes->SetLanes(1, 0);
      lanes->SetTurnLanes({}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway A4 https://www.openstreetmap.org/way/10589763
  // with two lanes
  ObjectFileRef a4Ref = databaseBuilder.AddMotorway(
    {GeoCoord(47.407877, 8.424234), GeoCoord(47.408958, 8.425497)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes, NameFeatureValue*) {
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });


  MockContext context(databaseBuilder.Build());

  {
    RouteDescription description;

    // route continue to A3
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a3Ref, 1);
    description.AddNode(0, 1, {a3Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest left lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::SlightLeft);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // leave motorway (as there is no connection to A3 in mock data, it would be motorway change)
    RouteDescription::MotorwayLeaveDescriptionRef motorwayLeaveDesc=std::dynamic_pointer_cast<RouteDescription::MotorwayLeaveDescription>(
      nodeIt->GetDescription(RouteDescription::MOTORWAY_LEAVE_DESC));
    REQUIRE(motorwayLeaveDesc);
    // TODO: add direction / lane information to MotorwayLeave and MotorwayChange descriptions
  }

  {
    RouteDescription description;

    // route continue to A4
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a4Ref, 1);
    description.AddNode(0, 1, {a4Ref}, ObjectFileRef(), 0);

    Postprocess(description, context);

    // should suggest two right lanes
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 2);
    REQUIRE(suggestedLanes->GetTurn() == LaneTurn::Through);

    ++nodeIt;
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(
      nodeIt->GetDescription(RouteDescription::DIRECTION_DESC));
    REQUIRE(directionDesc);
    REQUIRE(directionDesc->GetTurn()==RouteDescription::DirectionDescription::straightOn);
    // continue straight, no explicit turn
    REQUIRE(nodeIt->GetDescription(RouteDescription::TURN_DESC)==nullptr);
  }

}
