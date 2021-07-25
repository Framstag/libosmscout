
#include <TestMain.h>

TEST_CASE("Assignable 1") {
  auto b=std::byte(1u);

  REQUIRE(unsigned(b)==1u);
}

TEST_CASE("Assignable 255") {
  auto b=std::byte(255u);

  REQUIRE(unsigned(b)==255u);
}

TEST_CASE("We always get the last 8 bytes") {
  auto b=std::byte(4095u);  // 111111111111

  REQUIRE(unsigned(b)==255u);
}

TEST_CASE("No problem with unsigned and all bits set") {
  uint16_t val=65535;    // All bits set
  auto b=std::byte(val);

  REQUIRE(unsigned(b)==255u);
}

TEST_CASE("No problem with -1") {
  int16_t val=-1;    // All bits set
  auto b=std::byte(val);

  // This might in fact be implementation defined, but not on common hardware architectures
  REQUIRE(unsigned(b)==255u);
}
