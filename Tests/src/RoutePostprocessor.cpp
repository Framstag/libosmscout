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

#include <TestMain.h>

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
  MockDatabase(TypeConfigRef typeConfig, const TmpFileRef &wayFile): typeConfig(typeConfig), wayFile(wayFile)
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

class MockDatabaseBuilder {
private:
  TypeConfigRef typeConfig=std::make_shared<TypeConfig>();
  FileWriter writer;
  TypeInfoRef type;
  TmpFileRef wayFile=std::make_shared<TmpFile>("test-ways", ".dat");
  // needs to correspond to feature registration order
  static constexpr size_t laneFeatureIndex=0;
  static constexpr size_t accessFeatureIndex=1;

public:
  MockDatabaseBuilder()
  {
    osmscout::FeatureRef laneFeature = typeConfig->GetFeature(LanesFeature::NAME);
    assert(laneFeature);
    osmscout::FeatureRef accessFeature = typeConfig->GetFeature(AccessFeature::NAME);
    assert(accessFeature);

    type=std::make_shared<TypeInfo>("highway");
    // type->SetInternal()
    type->CanBeWay(true)
      .CanRouteCar(true)
      .AddFeature(laneFeature)
      .AddFeature(accessFeature);

    typeConfig->RegisterType(type);

    writer.Open(wayFile->path.string());
  }

  ~MockDatabaseBuilder()
  {
    if (writer.IsOpen()) {
      writer.Close();
    }
  }

  ObjectFileRef AddWay(const std::vector<GeoCoord> &coords, std::function<void(AccessFeatureValue*, LanesFeatureValue*)> setupFeatures)
  {
    Way way;

    FeatureValueBuffer featureValueBuffer;
    featureValueBuffer.SetType(type);

    setupFeatures(static_cast<AccessFeatureValue *>(featureValueBuffer.AllocateValue(accessFeatureIndex)),
                  static_cast<LanesFeatureValue *>(featureValueBuffer.AllocateValue(laneFeatureIndex)));

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
    return std::make_shared<MockDatabase>(typeConfig, wayFile);
  }
};

class MockContext: public osmscout::PostprocessorContext
{
private:
  MockDatabaseRef database;
  LanesFeatureValueReader lanesReader;
  AccessFeatureValueReader accessReader;

public:
  explicit MockContext(MockDatabaseRef database):
    database(database),
    lanesReader(*database->GetTypeConfig()),
    accessReader(*database->GetTypeConfig())
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

  const LanesFeatureValueReader& GetLaneReader(const DatabaseId &dbId) const override
  {
    assert(dbId==0);
    return lanesReader;
  }

  const AccessFeatureValueReader& GetAccessReader(const DatabaseId &dbId) const override
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
    assert(false);
    return osmscout::RouteDescription::NameDescriptionRef();
  }

  bool IsMotorwayLink([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsMotorway([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsMiniRoundabout([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsClockwise([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsRoundabout([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
    return false;
  }

  bool IsBridge([[maybe_unused]] const osmscout::RouteDescription::Node &node) const override
  {
    assert(false);
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

      if (fromNodeIndex>0) {
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

  osmscout::GeoCoord GetCoordinates([[maybe_unused]] const osmscout::RouteDescription::Node &node,
                                    [[maybe_unused]] size_t nodeIndex) const override
  {
    assert(false);
    return osmscout::GeoCoord();
  }

  vector<osmscout::DatabaseRef> GetDatabases() const override
  {
    assert(false);
    return vector<osmscout::DatabaseRef>();
  }
};

TEST_CASE("Suggest lanes on simple junction")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/#map=19/50.09267/14.53044
  MockDatabaseBuilder databaseBuilder;
  // street Českobrodská to east https://www.openstreetmap.org/way/936270761
  // with two lanes
  ObjectFileRef way1Ref=databaseBuilder.AddWay(
    {GeoCoord(50.09294, 14.52899), GeoCoord(50.09282, 14.52976)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  // continuation of Českobrodská https://www.openstreetmap.org/way/62143601
  // single lane
  ObjectFileRef way2Ref=databaseBuilder.AddWay(
    {GeoCoord(50.09282, 14.52976), GeoCoord(50.09263, 14.53118)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(1, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  // link between Českobrodská a Průmyslová https://www.openstreetmap.org/way/936270757
  // single lane
  ObjectFileRef way3Ref=databaseBuilder.AddWay(
    {GeoCoord(50.09282, 14.52976), GeoCoord(50.09261, 14.53093)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(1, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  RouteDescription description;

  MockContext context(databaseBuilder.Build());

  RoutePostprocessor::LanesPostprocessor lanesPostprocessor;
  RoutePostprocessor::SuggestedLanesPostprocessor postprocessor;

  {
    // route is going in direction to Průmyslová
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way3Ref, 1);
    description.AddNode(0, 1, {way3Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));

    // should suggest the right lane
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
  }

  {
    description.Clear();

    // route continue to the east
    description.AddNode(0, 0, {way1Ref}, way1Ref, 1);
    description.AddNode(0, 0, {way1Ref, way2Ref, way3Ref}, way2Ref, 1);
    description.AddNode(0, 1, {way2Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest left lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
  }
}


TEST_CASE("Suggest lanes on slightly complex junction")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/#map=19/50.077838/14.511968
  MockDatabaseBuilder databaseBuilder;
  // street Dřevčická to south https://www.openstreetmap.org/way/1308678203
  // with three lanes
  ObjectFileRef drevcicka1Ref=databaseBuilder.AddWay(
    {GeoCoord(50.078024, 14.511979), GeoCoord(50.077919, 14.511966)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 1);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Right}, {LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
    });

  // continuation of Dřevčická street to south https://www.openstreetmap.org/way/501228496
  // with two lanes
  ObjectFileRef drevcicka2Ref=databaseBuilder.AddWay(
    {GeoCoord(50.077919, 14.511966), GeoCoord(50.077766, 14.511963)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(1, 1);
      lanes->SetTurnLanes({LaneTurn::Left}, {LaneTurn::None});
      access->SetAccess(AccessFeatureValue::carForward | AccessFeatureValue::carBackward);
    });

  // north lanes of Černokostelecká, from east to west https://www.openstreetmap.org/way/22823101
  ObjectFileRef northCernokostelecka1Ref=databaseBuilder.AddWay(
    {GeoCoord(50.077845, 14.513782), GeoCoord(50.077919, 14.511966)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through_Right}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  // https://www.openstreetmap.org/way/1094576469
  ObjectFileRef northCernokostelecka2Ref=databaseBuilder.AddWay(
    {GeoCoord(50.077919, 14.511966), GeoCoord(50.077937, 14.510528)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  // south lanes of Černokostelecká, from west to east https://www.openstreetmap.org/way/1094576467
  ObjectFileRef southCernokostelecka1Ref=databaseBuilder.AddWay(
    {GeoCoord(50.077776, 14.510397), GeoCoord(50.077766, 14.511963)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Left, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  // https://www.openstreetmap.org/way/1094576468
  ObjectFileRef southCernokostelecka2Ref=databaseBuilder.AddWay(
    {GeoCoord(50.077766, 14.511963), GeoCoord(50.077685,14.513725)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      access->SetAccess(AccessFeatureValue::onewayForward | AccessFeatureValue::carForward);
    });

  MockContext context(databaseBuilder.Build());

  RouteDescription description;

  RoutePostprocessor::LanesPostprocessor lanesPostprocessor;
  lanesPostprocessor.Process(context, description);

  RoutePostprocessor::SuggestedLanesPostprocessor postprocessor;
  postprocessor.Process(context, description);

  { // route is going from west to east on Cernokostelecka
    description.Clear();
    description.AddNode(0, 0, {southCernokostelecka1Ref}, southCernokostelecka1Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, southCernokostelecka2Ref, 1);
    description.AddNode(0, 1, {southCernokostelecka2Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // TODO: should suggest right lane, through
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    // REQUIRE(suggestedLanes);
    // REQUIRE(suggestedLanes->GetFrom() == 1);
    // REQUIRE(suggestedLanes->GetTo() == 1);
  }

  { // route is going from west on Cernokostelecka to north on Drevcicka
    description.Clear();
    description.AddNode(0, 0, {southCernokostelecka1Ref}, southCernokostelecka1Ref, 1);
    description.AddNode(0, 1, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, drevcicka2Ref, 0);
    description.AddNode(0, 1, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka1Ref, 0);
    description.AddNode(0, 1, {drevcicka1Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest left lane, left turn
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
  }

  { // route is going from the east to west on Cernokostelecka
    description.Clear();
    description.AddNode(0, 0, {northCernokostelecka1Ref}, northCernokostelecka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, northCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka2Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // both lanes are usable, should provide no suggestion
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes==nullptr);
  }

  { // route is going from the east on Cernokostelecka to north on Drevcicka
    description.Clear();
    description.AddNode(0, 0, {northCernokostelecka1Ref}, northCernokostelecka1Ref, 1);
    description.AddNode(0, 1, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka1Ref, 0);
    description.AddNode(0, 0, {drevcicka1Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest right lane to the right
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 1);
  }

  { // route is going from the north from Drevcicka to the west on Cernokostelecka
    description.Clear();
    description.AddNode(0, 0, {drevcicka1Ref}, drevcicka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, northCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka2Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // TODO: should suggest right lane to the right
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    // REQUIRE(suggestedLanes);
    // REQUIRE(suggestedLanes->GetFrom() == 1);
    // REQUIRE(suggestedLanes->GetTo() == 1);
  }

  { // route is going from the north from Drevcicka to the east on Cernokostelecka
    description.Clear();
    description.AddNode(0, 0, {drevcicka1Ref}, drevcicka1Ref, 1);
    description.AddNode(0, 0, {northCernokostelecka1Ref, drevcicka1Ref, drevcicka2Ref, northCernokostelecka2Ref}, drevcicka2Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka1Ref, southCernokostelecka2Ref, drevcicka2Ref}, southCernokostelecka2Ref, 1);
    description.AddNode(0, 0, {southCernokostelecka2Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest left lane to the left
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
  }
}

TEST_CASE("Suggest lanes on A3/A4 highway split")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/query?lat=47.341840&lon=8.454964#map=19/47.341780/8.453880
  MockDatabaseBuilder databaseBuilder;
  // highway A3/A4 https://www.openstreetmap.org/way/1304846969
  // with three lanes
  ObjectFileRef a3a4Ref=databaseBuilder.AddWay(
    {GeoCoord(47.341814, 8.453848), GeoCoord(47.341844, 8.454816)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway A3 https://www.openstreetmap.org/way/38698216
  // with two lanes
  ObjectFileRef a3Ref=databaseBuilder.AddWay(
    {GeoCoord(47.341844, 8.454816), GeoCoord(47.341923, 8.456367)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway A4 https://www.openstreetmap.org/way/1205837033
  // with two lanes
  ObjectFileRef a4Ref=databaseBuilder.AddWay(
    {GeoCoord(47.341844, 8.454816), GeoCoord(47.341756, 8.455870)},
    [](AccessFeatureValue* access, LanesFeatureValue* lanes){
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  RouteDescription description;

  MockContext context(databaseBuilder.Build());

  RoutePostprocessor::LanesPostprocessor lanesPostprocessor;
  RoutePostprocessor::SuggestedLanesPostprocessor postprocessor;

  {
    description.Clear();

    // route continue to A4
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a4Ref, 1);
    description.AddNode(0, 1, {a4Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest right lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 2);
    REQUIRE(suggestedLanes->GetTo() == 2);
  }

  {
    description.Clear();

    // route continue to A3
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a3Ref, 1);
    description.AddNode(0, 1, {a3Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest two left through lanes
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 1);
  }
}

TEST_CASE("Suggest lanes on A3/A4 highway near Zurich")
{
  using namespace osmscout;

  // https://www.openstreetmap.org/way/116756494#map=18/47.407778/8.424867
  MockDatabaseBuilder databaseBuilder;

  // highway A3/A4 https://www.openstreetmap.org/way/116756494
  // with three lanes
  ObjectFileRef a3a4Ref = databaseBuilder.AddWay(
    {GeoCoord(47.407328, 8.423713), GeoCoord(47.407877, 8.424234)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes) {
      lanes->SetLanes(3, 0);
      lanes->SetTurnLanes({LaneTurn::SlightLeft, LaneTurn::Through, LaneTurn::Through}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway A3 https://www.openstreetmap.org/way/26834628
  // with one lane
  ObjectFileRef a3Ref = databaseBuilder.AddWay(
    {GeoCoord(47.407877, 8.424234), GeoCoord(47.409503, 8.425961)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes) {
      lanes->SetLanes(1, 0);
      lanes->SetTurnLanes({}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  // highway A4 https://www.openstreetmap.org/way/10589763
  // with two lanes
  ObjectFileRef a4Ref = databaseBuilder.AddWay(
    {GeoCoord(47.407877, 8.424234), GeoCoord(47.408958, 8.425497)},
    [](AccessFeatureValue *access, LanesFeatureValue *lanes) {
      lanes->SetLanes(2, 0);
      lanes->SetTurnLanes({LaneTurn::Through, LaneTurn::SlightRight}, {});
      access->SetAccess(AccessFeatureValue::carForward);
    });

  RouteDescription description;

  MockContext context(databaseBuilder.Build());

  RoutePostprocessor::LanesPostprocessor lanesPostprocessor;
  RoutePostprocessor::SuggestedLanesPostprocessor postprocessor;

  {
    description.Clear();

    // route continue to A3
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a3Ref, 1);
    description.AddNode(0, 1, {a3Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest left lane
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 0);
    REQUIRE(suggestedLanes->GetTo() == 0);
  }

  {
    description.Clear();

    // route continue to A4
    description.AddNode(0, 0, {a3a4Ref}, a3a4Ref, 1);
    description.AddNode(0, 0, {a3a4Ref, a3Ref, a4Ref}, a4Ref, 1);
    description.AddNode(0, 1, {a4Ref}, ObjectFileRef(), 0);

    lanesPostprocessor.Process(context, description);
    postprocessor.Process(context, description);

    // should suggest two right lanes
    auto nodeIt = description.Nodes().begin();
    auto suggestedLanes = std::dynamic_pointer_cast<RouteDescription::SuggestedLaneDescription>(
      nodeIt->GetDescription(RouteDescription::SUGGESTED_LANES_DESC));
    REQUIRE(suggestedLanes);
    REQUIRE(suggestedLanes->GetFrom() == 1);
    REQUIRE(suggestedLanes->GetTo() == 2);
  }

}
