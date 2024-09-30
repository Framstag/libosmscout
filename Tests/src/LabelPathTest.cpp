
#include <osmscoutmap/LabelPath.h>
#include <osmscout/util/Geometry.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

TEST_CASE("Angle variance with low angle")
{
  LabelPath path;
  path.AddPoint(osmscout::Vertex2D(0.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(10.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(20.0, 4.0));

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(45)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with low angle 2")
{
  LabelPath path;
  path.AddPoint(osmscout::Vertex2D(10.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(0.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(-10.0, 4.0));

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(25)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with low angle 3")
{
  LabelPath path;
  path.AddPoint(osmscout::Vertex2D(-10.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(0.0, 0.0));
  path.AddPoint(osmscout::Vertex2D(10.0, 4.0));

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(25)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with high angle")
{
  LabelPath path;
  path.AddPoint(osmscout::Vertex2D(10.0,0.0));
  path.AddPoint(osmscout::Vertex2D(0.0,0.0));
  path.AddPoint(osmscout::Vertex2D(10.0,5.0));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(45)));
}
