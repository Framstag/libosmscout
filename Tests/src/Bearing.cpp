#include <iostream>

#include <osmscout/util/Geometry.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("North") {
  osmscout::GeoCoord a(-1.0,0.0);
  osmscout::GeoCoord b(1.0,0.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;
  REQUIRE(description=="N");
}

TEST_CASE("East") {
  osmscout::GeoCoord a(0.0,-1.0);
  osmscout::GeoCoord b(0.0,1.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="E");
}

TEST_CASE("South") {
  osmscout::GeoCoord a(1.0,0.0);
  osmscout::GeoCoord b(-1.0,0.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="S");
}

TEST_CASE("West") {
  osmscout::GeoCoord a(0.0,1.0);
  osmscout::GeoCoord b(0.0,-1.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="W");
}

TEST_CASE("East 2") {
  osmscout::GeoCoord a(0.0,10.0);
  osmscout::GeoCoord b(0.0,15.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="E");
}

TEST_CASE("West 2") {
  osmscout::GeoCoord a(0.0,15.0);
  osmscout::GeoCoord b(0.0,10.0);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="W");
}

TEST_CASE("Dortmund 1") {
  osmscout::GeoCoord a(51.57162,7.45882);
  osmscout::GeoCoord b(51.57141,7.46007);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="E");
}

TEST_CASE("Dortmund 2") {
  osmscout::GeoCoord a(51.57251,7.46506);
  osmscout::GeoCoord b(51.57269,7.46594);

  double bearing=osmscout::GetSphericalBearingInitial(a,b);
  std::string description=osmscout::BearingDisplayString(bearing);

  std::cout << a.GetLat()-b.GetLat() << " " << a.GetLon()-b.GetLon() << " " << bearing << " " << description << std::endl;

  REQUIRE(description=="E");
}

