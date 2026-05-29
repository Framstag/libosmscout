#include <iostream>
#include <unordered_map>

#include <osmscout/TypeConfig.h>

#include <osmscout/feature/HighwayMilestoneFeature.h>

#include <osmscout/util/TagErrorReporter.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("HighwayMilestone Parse converts OSM km tag to internal meters")
{
  osmscout::SilentTagErrorReporter reporter;
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  osmscout::TagId tagDistance=typeConfig.GetTagRegistry().RegisterTag("distance");
  osmscout::TagId tagRef=typeConfig.GetTagRegistry().RegisterTag("ref");

  tags[tagDistance]="35";
  tags[tagRef]="A2";

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  milestoneFeature->Parse(reporter,
                          typeConfig.GetTagRegistry(),
                          featureInstance,
                          osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                          tags,
                          buffer);

  osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

  const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

  REQUIRE(value!=nullptr);

  // OSM distance=35 (km) should be stored as 35000 (meters)
  REQUIRE(value->GetDistance()==35000);
  REQUIRE(value->GetRef()=="A2");
}

TEST_CASE("HighwayMilestone GetLabel returns distance in kilometers")
{
  osmscout::HighwayMilestoneFeatureValue value(35000,"A2");

  osmscout::Locale locale;
  locale.SetUnitsSeparator(" ");

  REQUIRE(value.GetLabel(locale,0)=="35 km");
}

TEST_CASE("HighwayMilestone Parse converts decimal OSM km to internal meters")
{
  osmscout::SilentTagErrorReporter reporter;
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  osmscout::TagId tagDistance=typeConfig.GetTagRegistry().RegisterTag("distance");
  osmscout::TagId tagRef=typeConfig.GetTagRegistry().RegisterTag("ref");

  tags[tagDistance]="35.5";
  tags[tagRef]="A2";

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  milestoneFeature->Parse(reporter,
                          typeConfig.GetTagRegistry(),
                          featureInstance,
                          osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                          tags,
                          buffer);

  osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

  const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

  REQUIRE(value!=nullptr);

  // OSM distance=35.5 (km) should be stored as 35500 (meters, decimal truncated after multiplication)
  REQUIRE(value->GetDistance()==35500);
}

TEST_CASE("HighwayMilestone Parse logs warning on invalid distance but still allocates feature value")
{
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  struct TestCase {
    std::string distance;
    std::string label;
  };

  const TestCase cases[]={
    {"35.0 mi", "unit suffix"},
    {"45 + 5",  "combined km+m format"},
    {"35,5",    "comma separator"}
  };

  for (const auto& tc : cases) {
    osmscout::SilentTagErrorReporter reporter;
    std::unordered_map<osmscout::TagId,std::string> tags;

    osmscout::TagId tagDistance=typeConfig.GetTagRegistry().RegisterTag("distance");
    osmscout::TagId tagRef=typeConfig.GetTagRegistry().RegisterTag("ref");

    tags[tagDistance]=tc.distance;
    tags[tagRef]="A2";

    osmscout::FeatureValueBuffer buffer;
    buffer.SetType(testType);

    milestoneFeature->Parse(reporter,
                            typeConfig.GetTagRegistry(),
                            featureInstance,
                            osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                            tags,
                            buffer);

    osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

    const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

    // Feature value should still be allocated with distance=0 and ref preserved
    REQUIRE(value!=nullptr);
    REQUIRE(value->GetDistance()==0);
    REQUIRE(value->GetRef()=="A2");
  }
}

TEST_CASE("HighwayMilestone Parse allocates feature with only ref tag")
{
  osmscout::SilentTagErrorReporter reporter;
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  osmscout::TagId tagRef=typeConfig.GetTagRegistry().RegisterTag("ref");

  tags[tagRef]="A2";

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  milestoneFeature->Parse(reporter,
                          typeConfig.GetTagRegistry(),
                          featureInstance,
                          osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                          tags,
                          buffer);

  osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

  const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

  REQUIRE(value!=nullptr);
  REQUIRE(value->GetRef()=="A2");
  REQUIRE(value->GetDistance()==0);
}

TEST_CASE("HighwayMilestone Parse allocates feature with only distance tag")
{
  osmscout::SilentTagErrorReporter reporter;
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  osmscout::TagId tagDistance=typeConfig.GetTagRegistry().RegisterTag("distance");

  tags[tagDistance]="35";

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  milestoneFeature->Parse(reporter,
                          typeConfig.GetTagRegistry(),
                          featureInstance,
                          osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                          tags,
                          buffer);

  osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

  const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

  REQUIRE(value!=nullptr);
  REQUIRE(value->GetDistance()==35000);
  REQUIRE(value->GetRef()=="");
}

TEST_CASE("HighwayMilestone Parse allocates feature with no sub-tags")
{
  osmscout::SilentTagErrorReporter reporter;
  osmscout::TypeConfig typeConfig;
  osmscout::TypeInfoRef testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef milestoneFeature;
  size_t featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  milestoneFeature=typeConfig.GetFeature(osmscout::HighwayMilestoneFeature::NAME);

  testType->CanRouteFoot(false);
  testType->CanRouteBicycle(false);
  testType->CanRouteCar(false);
  testType->AddFeature(milestoneFeature);

  typeConfig.RegisterType(testType);

  REQUIRE(testType->GetFeature(osmscout::HighwayMilestoneFeature::NAME,
                               featureInstanceIndex));

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  milestoneFeature->Parse(reporter,
                          typeConfig.GetTagRegistry(),
                          featureInstance,
                          osmscout::ObjectOSMRef(1,osmscout::osmRefNode),
                          tags,
                          buffer);

  osmscout::HighwayMilestoneFeatureValueReader valueReader(typeConfig);

  const osmscout::HighwayMilestoneFeatureValue *value=valueReader.GetValue(buffer);

  REQUIRE(value!=nullptr);
  REQUIRE(value->GetDistance()==0);
  REQUIRE(value->GetRef()=="");
  REQUIRE(value->GetCarriagewayRef()=="");
  REQUIRE(value->GetMarker()=="");
}

TEST_CASE("HighwayMilestone round-trip serialization with only ref set")
{
  osmscout::HighwayMilestoneFeatureValue original;
  original.SetRef("A2");

  REQUIRE(original.GetDistance()==0);
  REQUIRE(original.GetRef()=="A2");
  REQUIRE(original.GetCarriagewayRef()=="");
  REQUIRE(original.GetMarker()=="");

  // Round-trip through serialization
  osmscout::FileWriter writer;
  writer.Open("/tmp/highway_milestone_minimal_roundtrip.dat");

  original.Write(writer);
  writer.Close();

  osmscout::FileScanner scanner;
  scanner.Open("/tmp/highway_milestone_minimal_roundtrip.dat", osmscout::FileScanner::Sequential, false);

  osmscout::HighwayMilestoneFeatureValue restored;
  restored.Read(scanner);
  scanner.Close();

  REQUIRE(restored.GetDistance()==original.GetDistance());
  REQUIRE(restored.GetRef()==original.GetRef());
  REQUIRE(restored.GetCarriagewayRef()==original.GetCarriagewayRef());
  REQUIRE(restored.GetMarker()==original.GetMarker());
}

TEST_CASE("HighwayMilestone round-trip serialization with all defaults")
{
  osmscout::HighwayMilestoneFeatureValue original;

  REQUIRE(original.GetDistance()==0);
  REQUIRE(original.GetRef()=="");
  REQUIRE(original.GetCarriagewayRef()=="");
  REQUIRE(original.GetMarker()=="");

  // Round-trip through serialization
  osmscout::FileWriter writer;
  writer.Open("/tmp/highway_milestone_default_roundtrip.dat");

  original.Write(writer);
  writer.Close();

  osmscout::FileScanner scanner;
  scanner.Open("/tmp/highway_milestone_default_roundtrip.dat", osmscout::FileScanner::Sequential, false);

  osmscout::HighwayMilestoneFeatureValue restored;
  restored.Read(scanner);
  scanner.Close();

  REQUIRE(restored.GetDistance()==original.GetDistance());
  REQUIRE(restored.GetRef()==original.GetRef());
  REQUIRE(restored.GetCarriagewayRef()==original.GetCarriagewayRef());
  REQUIRE(restored.GetMarker()==original.GetMarker());
}
