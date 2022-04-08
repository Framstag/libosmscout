#include <osmscout/GeoCoord.h>
#include <osmscout/util/Time.h>
#include <osmscout/util/String.h>
#include <osmscout/util/SunriseSunset.h>

#include <TestMain.h>

// Prints sunrise and sunset time in specific timezone.
// Note that printed string is ISO8601 with UTC timezone - it is not correct (when tzOffset != 0)
// but we don't have proper time library...
std::string ToString(const osmscout::SunriseSunsetRes &sunriseSunset, int tzOffset)
{
  std::ostringstream stream;
  if (sunriseSunset) {
    auto offset = std::chrono::hours(tzOffset);
    stream << osmscout::TimestampToISO8601TimeString(std::get<0>(sunriseSunset.value()) + offset)
           << " - "
           << osmscout::TimestampToISO8601TimeString(std::get<1>(sunriseSunset.value()) + offset);
  } else {
    stream << "nullopt";
  }
  return stream.str();
}

std::string SunriseSunsetString(const std::string &day,
                                const osmscout::GeoCoord &location,
                                int tzOffset)
{
  osmscout::Timestamp ts;
  REQUIRE(osmscout::ParseISO8601TimeString(day, ts));
  auto sunriseSunset = osmscout::GetSunriseSunset(location, ts);
  return ToString(sunriseSunset, tzOffset);
}

osmscout::Timestamp Ts(const std::string &str)
{
  osmscout::Timestamp ts;
  REQUIRE(osmscout::ParseISO8601TimeString(str, ts));
  return ts;
}

std::string ToString(const osmscout::Timestamp &a, const osmscout::Timestamp &b)
{
  return osmscout::TimestampToISO8601TimeString(a)
         + " - "
         + osmscout::TimestampToISO8601TimeString(b);
}

bool SunriseSunsetTest(const osmscout::Timestamp &day,
                       const osmscout::GeoCoord &location,
                       int tzOffset,
                       const osmscout::Timestamp &sunrise,
                       const osmscout::Timestamp &sunset)
{
  auto sunriseSunset = osmscout::GetSunriseSunset(location, day);
  REQUIRE(sunriseSunset);
  auto offset = std::chrono::hours(tzOffset);
  using namespace std::chrono;
  std::cout << ToString(sunriseSunset, tzOffset) << " <> " << ToString(sunrise, sunset) << std::endl;
  auto sunriseDiff = duration_cast<seconds>((std::get<0>(sunriseSunset.value()) + offset) - sunrise);
  auto sunsetDiff = duration_cast<seconds>((std::get<1>(sunriseSunset.value()) + offset) - sunset);
  return std::abs(sunriseDiff.count()) < 180 &&
         std::abs(sunsetDiff.count()) < 180;
}

TEST_CASE("Compute Prague sunrise and sunset at winter solstice") {
  std::string winterSolstice="2021-12-21T12:00:00.0Z";
  osmscout::GeoCoord pragueLoc(50.083, 14.422);
  REQUIRE(SunriseSunsetString(winterSolstice, pragueLoc, +1) == "2021-12-21T07:59:58.0Z - 2021-12-21T16:03:24.0Z");
}

TEST_CASE("Polar night at Tromso at winter solstice") {
  std::string winterSolstice="2021-12-21T12:00:00.0Z";
  osmscout::GeoCoord tromsoLoc(69.6523, 18.9753);
  REQUIRE(SunriseSunsetString(winterSolstice, tromsoLoc, +1) == "nullopt");
}

TEST_CASE("Compute sunrise and sunset for some date and locations") {
  REQUIRE(SunriseSunsetTest(Ts("2013-01-20T12:00:00.0Z"), osmscout::GeoCoord(34.0522, -118.2437), -8,
                            Ts("2013-01-20T06:57:00.0Z"), Ts("2013-01-20T17:11:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-01-20T12:00:00.0Z"), osmscout::GeoCoord(48.8567, 2.351), +1,
                            Ts("2013-01-20T08:35:00.0Z"), Ts("2013-01-20T17:28:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2012-12-25T12:00:00.0Z"), osmscout::GeoCoord(-33.86, 151.2111), +11,
                            Ts("2012-12-25T05:43:00.0Z"), Ts("2012-12-25T20:07:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-05-01T12:00:00.0Z"), osmscout::GeoCoord(35.6938, 139.7036), +9,
                            Ts("2013-05-01T04:49:00.0Z"), Ts("2013-05-01T18:27:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-06-05T12:00:00.0Z"), osmscout::GeoCoord(53.3441, -6.2675), +1,
                            Ts("2013-06-05T05:01:00.0Z"), Ts("2013-06-05T21:46:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-06-22T12:00:00.0Z"), osmscout::GeoCoord(41.8781, -87.6298), -5,
                            Ts("2013-06-22T05:16:00.0Z"), Ts("2013-06-22T20:29:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2015-08-27T12:00:00.0Z"), osmscout::GeoCoord(21.3069, -157.8583), -10,
                            Ts("2015-08-27T06:13:00.0Z"), Ts("2015-08-27T18:53:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-05-01T12:00:00.0Z"), osmscout::GeoCoord(-34.6092, -58.3732), -3,
                            Ts("2013-05-01T07:29:00.0Z"), Ts("2013-05-01T18:12:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-10-19T12:00:00.0Z"), osmscout::GeoCoord(-34.6092, -58.3732), -3,
                            Ts("2013-10-19T06:07:00.0Z"), Ts("2013-10-19T19:11:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-01-26T12:00:00.0Z"), osmscout::GeoCoord(-34.6092, -58.3732), -3,
                            Ts("2013-01-26T06:07:00.0Z"), Ts("2013-01-26T20:04:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-10-20T12:00:00.0Z"), osmscout::GeoCoord(-34.6092, -58.3732), -3,
                            Ts("2013-10-20T06:05:00.0Z"), Ts("2013-10-20T19:11:00.0Z")));
  REQUIRE(SunriseSunsetTest(Ts("2013-10-31T12:00:00.0Z"), osmscout::GeoCoord(-34.6092, -58.3732), -3,
                            Ts("2013-10-31T05:53:00.0Z"), Ts("2013-10-31T19:21:00.0Z")));
}
