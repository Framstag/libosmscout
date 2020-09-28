#include <iostream>

#include <osmscout/util/NumberSet.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE()
{
  osmscout::NumberSet set;

  REQUIRE_FALSE(set.IsSet(0));

  REQUIRE_FALSE(set.IsSet(1));

  set.Set(1);

  REQUIRE(set.IsSet(1));

  set.Set(255);

  REQUIRE(set.IsSet(255));

  set.Set(256);

  REQUIRE(set.IsSet(256));

  for (size_t i=256; i<256*256; i++) {
    set.Set(i);

    REQUIRE(set.IsSet(i));
  }
}
