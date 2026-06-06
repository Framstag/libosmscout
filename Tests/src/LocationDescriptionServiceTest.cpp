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
  REQUIRE(desc.GetPreviousMilestoneDistance() == 0);
  REQUIRE(desc.GetPreviousMilestoneRef().empty());
}


TEST_CASE("LocationHighwayMilestoneDescription at-distance constructor with milestone data")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(50);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(45.0);
  uint32_t previousMilestoneDistance=35;
  std::string previousMilestoneRef="A2";
  std::string previousMilestoneCarriagewayRef="A2C";

  osmscout::LocationHighwayMilestoneDescription desc(object, buffer, distance, bearing,
                                                     previousMilestoneDistance, previousMilestoneRef,
                                                     previousMilestoneCarriagewayRef);

  REQUIRE_FALSE(desc.IsAtPlace());
  REQUIRE(desc.GetDistance() == distance);
  REQUIRE(desc.GetPreviousMilestoneDistance() == 35);
  REQUIRE(desc.GetPreviousMilestoneRef() == "A2");
  REQUIRE(desc.GetPreviousMilestoneCarriagewayRef() == "A2C");
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

TEST_CASE("LocationHighwayMilestoneDescription between two milestones")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(50);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(45.0);

  uint32_t previousDistance=35;
  std::string previousRef="A2";
  uint32_t nextDistance=50;
  std::string nextRef="A3";

  osmscout::LocationHighwayMilestoneDescription desc(
      object, buffer, distance, bearing,
      previousDistance, previousRef, "",
      nextDistance, nextRef, "");

  REQUIRE(desc.IsBetweenMilestones());
  REQUIRE(desc.GetPreviousMilestoneDistance() == 35);
  REQUIRE(desc.GetPreviousMilestoneRef() == "A2");
  REQUIRE(desc.GetNextMilestoneDistance() == 50);
  REQUIRE(desc.GetNextMilestoneRef() == "A3");
}

TEST_CASE("LocationHighwayMilestoneDescription before first milestone fallback")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(30);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(0.0);

  // Only next milestone set (previous is default)
  osmscout::LocationHighwayMilestoneDescription desc(object, buffer, distance, bearing,
                                                     0, "", "",
                                                     45, "A3", "");

  REQUIRE_FALSE(desc.IsBetweenMilestones());
  // desc uses next as the nearest, but data model stores both
  REQUIRE(desc.GetPreviousMilestoneRef() == "");
  REQUIRE(desc.GetNextMilestoneRef() == "A3");
}

TEST_CASE("LocationHighwayMilestoneDescription after last milestone fallback")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(20);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(180.0);

  // Only previous milestone set (next is default)
  osmscout::LocationHighwayMilestoneDescription desc(object, buffer, distance, bearing,
                                                     22, "A2", "",
                                                     0, "", "");

  REQUIRE_FALSE(desc.IsBetweenMilestones());
  REQUIRE(desc.GetPreviousMilestoneRef() == "A2");
  REQUIRE(desc.GetNextMilestoneRef() == "");
}

TEST_CASE("LocationHighwayMilestoneDescription exactly at milestone")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(0);
  osmscout::Bearing bearing;

  // previous and next point to same milestone (same ref + distance)
  osmscout::LocationHighwayMilestoneDescription desc(
      object, buffer, distance, bearing,
      35, "A2", "",
      35, "A2", "");

  REQUIRE_FALSE(desc.IsBetweenMilestones());
}

TEST_CASE("LocationHighwayMilestoneDescription only one milestone on way")
{
  osmscout::ObjectFileRef object;
  osmscout::FeatureValueBufferRef buffer;

  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Meter>(15);
  osmscout::Bearing bearing=osmscout::Bearing::Degrees(90.0);

  // Single-milestone: only previous set (next empty)
  osmscout::LocationHighwayMilestoneDescription desc(object, buffer, distance, bearing,
                                                     12, "B1", "");

  REQUIRE_FALSE(desc.IsBetweenMilestones());
  REQUIRE(desc.GetPreviousMilestoneRef() == "B1");
  REQUIRE(desc.GetNextMilestoneRef() == "");
}
