
#include <osmscout/LabelPath.h>
#include <osmscout/util/Geometry.h>

#include <TestMain.h>

using namespace osmscout;

TEST_CASE("Angle variance with low angle")
{
  LabelPath path;
  path.AddPoint(0, 0);
  path.AddPoint(10, 0);
  path.AddPoint(20, 4);

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(45)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with low angle 2")
{
  LabelPath path;
  path.AddPoint(10, 0);
  path.AddPoint(0, 0);
  path.AddPoint(-10, 4);

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(25)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with low angle 3")
{
  LabelPath path;
  path.AddPoint(-10, 0);
  path.AddPoint(0, 0);
  path.AddPoint(10, 4);

  REQUIRE(path.TestAngleVariance(0, 20, DegToRad(25)));
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(10)));
}

TEST_CASE("Angle variance with high angle")
{
  LabelPath path;
  path.AddPoint(10,0);
  path.AddPoint(0,0);
  path.AddPoint(10,5);
  REQUIRE_FALSE(path.TestAngleVariance(0, 20, DegToRad(45)));
}
