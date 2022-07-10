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
  REQUIRE(std::byte(256u*256u)==std::byte(0x00u));
  // A random number, we take the lower byte
  REQUIRE(std::byte(1001)==std::byte(0xe9u));
}

TEST_CASE("std::byte and negative numbers") {
  int16_t positiveNumber=10000; // 0x2710u

  REQUIRE(std::byte(positiveNumber >> 8)==std::byte(0x027u));

  int16_t negativeNumber=-10000; // 0xd8f0u, 2 complement

  REQUIRE(std::byte(negativeNumber >> 8)==std::byte(0x0d8u));
}

