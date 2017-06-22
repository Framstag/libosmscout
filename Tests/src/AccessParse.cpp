#include <iostream>
#include <sstream>
#include <unordered_map>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/util/TagErrorReporter.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

std::string AccessToString(uint8_t access)
{
  std::stringstream buffer;
  bool              empty=true;

  if (access & osmscout::AccessFeatureValue::onewayBackward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "OnewayBackward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::onewayForward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "OnewayForward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::footBackward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "FootBackward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::footForward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "FootForward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::bicycleBackward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "BicycleBackward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::bicycleForward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "BicycleForward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::carBackward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "CarBackward";

    empty=false;
  }

  if (access & osmscout::AccessFeatureValue::carForward) {
    if (!empty) {
      buffer << "|";
    }

    buffer << "CarForward";

    empty=false;
  }

  return buffer.str();
}

void CheckParseSuccess(bool canFoot,
                       bool canBicycle,
                       bool canCar,
                       const std::unordered_map<std::string,std::string>& stringTags,
                       uint8_t& actualAccessValue)
{
  osmscout::SilentTagErrorReporter                reporter;
  osmscout::TypeConfig                            typeConfig;
  osmscout::TypeInfoRef                           testType=std::make_shared<osmscout::TypeInfo>("TestType");
  osmscout::FeatureRef                            accessFeature;
  size_t                                          featureInstanceIndex;
  std::unordered_map<osmscout::TagId,std::string> tags;

  for (const auto &entry : stringTags) {
    osmscout::TagId tagId=typeConfig.RegisterTag(entry.first);

    tags[tagId]=entry.second;
  }

  accessFeature=typeConfig.GetFeature(osmscout::AccessFeature::NAME);

  testType->CanRouteFoot(canFoot);
  testType->CanRouteBicycle(canBicycle);
  testType->CanRouteCar(canCar);
  testType->AddFeature(accessFeature);

  typeConfig.RegisterType(testType);

  if (!testType->GetFeature(osmscout::AccessFeature::NAME,
                            featureInstanceIndex)) {
    std::cout << "FAIL: internal error" << std::endl;

    return;
  }

  osmscout::FeatureInstance featureInstance=testType->GetFeature(featureInstanceIndex);

  osmscout::FeatureValueBuffer buffer;

  buffer.SetType(testType);

  accessFeature->Parse(reporter,
                       typeConfig,
                       featureInstance,
                       osmscout::ObjectOSMRef(1,osmscout::osmRefWay),
                       tags,
                       buffer);

  osmscout::AccessFeatureValueReader accessValueReader(typeConfig);

  osmscout::AccessFeatureValue *accessFeatureValue=accessValueReader.GetValue(buffer);

  if (accessFeatureValue!=NULL) {
    actualAccessValue=accessFeatureValue->GetAccess();
  }
  else {
    actualAccessValue=testType->GetDefaultAccess();
  }
}

TEST_CASE("Highway=path (FOOT, BICYCLE) with bicycle explicitly forbidden") {
  std::unordered_map<std::string,std::string> tags;
  uint8_t expectedValue=osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward;
  uint8_t actualValue=0;

  tags["access"]="agricultural";
  tags["bicycle"]="no";
  tags["foot"]="yes";

  CheckParseSuccess(true,
                    true,
                    false,
                    tags,
                    actualValue);

  REQUIRE(AccessToString(actualValue)==AccessToString(expectedValue));
}

TEST_CASE("Highway=footway (FOOT) with bicycle explicitly allowed") {
  std::unordered_map<std::string,std::string> tags;
  uint8_t expectedValue=osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward|osmscout::AccessFeatureValue::bicycleForward|osmscout::AccessFeatureValue::bicycleBackward;
  uint8_t actualValue=0;

  tags["bicycle"]="yes";

  CheckParseSuccess(true,
                    false,
                    false,
                    tags,
                    actualValue);

  REQUIRE(AccessToString(actualValue)==AccessToString(expectedValue));
}

TEST_CASE("Highway=track (FOOT) with bicycle explicitly forbidden") {
  std::unordered_map<std::string,std::string> tags;
  uint8_t expectedValue=osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward;
  uint8_t actualValue=0;

  tags["bicycle"]="no";

  CheckParseSuccess(true,
                    true,
                    false,
                    tags,
                    actualValue);

  REQUIRE(AccessToString(actualValue)==AccessToString(expectedValue));
}

TEST_CASE("Highway=motorway (CAR) with foot and bicycle explicitly forbidden") {
  std::unordered_map<std::string,std::string> tags;
  uint8_t expectedValue=osmscout::AccessFeatureValue::onewayForward|osmscout::AccessFeatureValue::carForward;
  uint8_t actualValue=0;

  tags["foot"]="no";
  tags["bicycle"]="no";
  tags["oneway"]="yes";

  CheckParseSuccess(false,
                    false,
                    true,
                    tags,
                    actualValue);

  REQUIRE(AccessToString(actualValue)==AccessToString(expectedValue));
}

TEST_CASE("Highway=motorway (CAR) with no access at all, but oneway") {
  std::unordered_map<std::string,std::string> tags;
  uint8_t expectedValue=osmscout::AccessFeatureValue::onewayForward|osmscout::AccessFeatureValue::carForward;
  uint8_t actualValue=0;

  tags["access"]="no";
  tags["oneway"]="yes";
  tags["bus"]="destination";
  tags["psv"]="yes";
  tags["motor_vehicle"]="private";

  CheckParseSuccess(false,
                    false,
                    false,
                    tags,
                    actualValue);

  REQUIRE(AccessToString(actualValue)==AccessToString(expectedValue));
}
