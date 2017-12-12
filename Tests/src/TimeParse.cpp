#include <osmscout/util/String.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Parse ISO8601 time string") {
  osmscout::Timestamp ts;
  std::string testString="2017-03-12T14:31:56.123Z";
  REQUIRE(osmscout::ParseISO8601TimeString(testString, ts));
  REQUIRE(ts.time_since_epoch().count()==1489329116123);
  REQUIRE(osmscout::TimestampToISO8601TimeString(ts)==testString);
}
