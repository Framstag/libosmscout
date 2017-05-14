#include <iostream>
#include <sstream>
#include <unordered_map>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/util/TagErrorReporter.h>

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

bool CheckParseSuccess(bool canFoot,
                       bool canBicycle,
                       bool canCar,
                       uint8_t expectedAccessValue,
                       const std::unordered_map<std::string,std::string>& stringTags)
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

    return false;
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

  uint8_t accessValue;

  if (accessFeatureValue!=NULL) {
    accessValue=accessFeatureValue->GetAccess();
  }
  else {
    accessValue=testType->GetDefaultAccess();
  }

  if (expectedAccessValue==accessValue) {
    std::cout << "OK" << std::endl;
    return true;
  }
  else {
    std::cout << "FAIL: " << "expected " << AccessToString(expectedAccessValue) << " got " << AccessToString(accessValue) << std::endl;
    return false;
  }
}

int main()
{
  int errors=0;

  std::unordered_map<std::string,std::string> tags;

  // Highway=path (FOOT, BICYCLE) with bicycle explicitly forbidden

  tags.clear();
  tags["access"]="agricultural";
  tags["bicycle"]="no";
  tags["foot"]="yes";

  if (!CheckParseSuccess(true,
                         true,
                         false,
                         osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward,
                         tags)) {
    errors++;
  }

  // Highway=footway (FOOT) with bicycle explicitly allowed

  tags.clear();
  tags["bicycle"]="yes";

  if (!CheckParseSuccess(true,
                         false,
                         false,
                         osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward|
                         osmscout::AccessFeatureValue::bicycleForward|osmscout::AccessFeatureValue::bicycleBackward,
                         tags)) {
    errors++;
  }

  // Highway=track (FOOT) with bicycle explicitly forbidden

  tags.clear();
  tags["bicycle"]="no";

  if (!CheckParseSuccess(true,
                         true,
                         false,
                         osmscout::AccessFeatureValue::footForward|osmscout::AccessFeatureValue::footBackward,
                         tags)) {
    errors++;
  }

  // Highway=motorway (CAR) with foot and bicycle explicitly forbidden

  tags.clear();
  tags["foot"]="no";
  tags["bicycle"]="no";
  tags["oneway"]="yes";

  if (!CheckParseSuccess(false,
                         false,
                         true,
                         osmscout::AccessFeatureValue::onewayForward|osmscout::AccessFeatureValue::carForward,
                         tags)) {
    errors++;
  }

  // Highway=motorway (CAR) with no access at all, but oneway

  tags.clear();
  tags["access"]="no";
  tags["oneway"]="yes";
  tags["bus"]="destination";
  tags["psv"]="yes";
  tags["motor_vehicle"]="private";

  if (!CheckParseSuccess(false,
                         false,
                         false,
                         osmscout::AccessFeatureValue::onewayForward|osmscout::AccessFeatureValue::carForward,
                         tags)) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
