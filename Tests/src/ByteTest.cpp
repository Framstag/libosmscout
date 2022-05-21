#include <cstddef>

#include <TestMain.h>

TEST_CASE("std::byte initialisation") {
  // Identity
  REQUIRE(std::byte(0xffu)==std::byte(0xffu));
  // Int>255, we take the lower byte
  REQUIRE(std::byte(255+1)==std::byte(0x00u));
  // Int>255, we take the lower byte
  REQUIRE(std::byte(256)==std::byte(0x00u));
  // Shifting beyong 8 bits, we take the lower byte
  REQUIRE(std::byte(1u >> 8)==std::byte(0x00u));
  // Two bytes with 0xff, we take the lower byte
  REQUIRE(std::byte(256*256)==std::byte(0x00u));
  // A rando number, we take the lower byte
  REQUIRE(std::byte(1001)==std::byte(0xe9u));
}
