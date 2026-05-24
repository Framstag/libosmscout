#include <iostream>

#include <osmscout/location/LocationDescriptionService.h>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("LocationHighwayMilestoneDescription at-place constructor")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::LocationHighwayMilestoneDescription desc(object, buffer);

  REQUIRE(desc.IsAtPlace());
  REQUIRE(desc.GetDistance() == osmscout::Distance::Zero());
  REQUIRE(desc.GetMilestoneDistance() == 0);
  REQUIRE(desc.GetMilestoneRef().empty());
}


TEST_CASE("LocationHighwayMilestoneDescription at-distance constructor with milestone data")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(50);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(45.0);
  uint32_t milestoneDistance=35;
  std::string milestoneRef="A2";
  std::string milestoneCarriagewayRef="A2C";

  osmscout::LocationHighwayMilestoneDescription desc(object, buffer, distance, bearing,
                                                     milestoneDistance, milestoneRef,
                                                     milestoneCarriagewayRef);

  REQUIRE_FALSE(desc.IsAtPlace());
  REQUIRE(desc.GetDistance() == distance);
  REQUIRE(desc.GetMilestoneDistance() == 35);
  REQUIRE(desc.GetMilestoneRef() == "A2");
  REQUIRE(desc.GetMilestoneCarriagewayRef() == "A2C");
}
TEST_CASE("LocationDescription highway milestone setter/getter")
{
  osmscout::LocationDescription desc;

  REQUIRE(desc.GetHighwayMilestoneDescription() == nullptr);

  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  auto milestoneDesc=std::make_shared<osmscout::LocationHighwayMilestoneDescription>(object, buffer);
  desc.SetHighwayMilestoneDescription(milestoneDesc);

  REQUIRE(desc.GetHighwayMilestoneDescription() != nullptr);
  REQUIRE(desc.GetHighwayMilestoneDescription()->IsAtPlace());
}

TEST_CASE("LocationDescription highway milestone null when not set")
{
  osmscout::LocationDescription desc;
  REQUIRE(desc.GetHighwayMilestoneDescription() == nullptr);
}
