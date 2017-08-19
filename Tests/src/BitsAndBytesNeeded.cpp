#include <iostream>

#include <osmscout/util/Number.h>

int errors=0;

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

bool CheckBitsNeededToEncodeNumber(unsigned long number, uint8_t expectedBits)
{
  uint8_t actualBits=osmscout::BitsNeededToEncodeNumber(number);

  if (actualBits!=expectedBits) {
    std::cerr << "Error in 'BitsNeededToEncodeNumber' expected " << (size_t)expectedBits << " actual " << (size_t)actualBits << std::endl;
    return false;
  }

  return true;
}

bool CheckBytesNeededToEncodeNumber(unsigned long number, uint8_t expectedBytes)
{
  uint8_t actualBytes=osmscout::BytesNeededToEncodeNumber(number);

  if (actualBytes!=expectedBytes) {
    std::cerr << "Error in 'BitsNeededToEncodeNumber' expected " << (size_t)expectedBytes << " actual " << (size_t)actualBytes << std::endl;
    return false;
  }

  return true;
}

TEST_CASE("CheckBitsNeededToEncodeNumber") {
  REQUIRE(CheckBitsNeededToEncodeNumber(0,1));
  REQUIRE(CheckBitsNeededToEncodeNumber(1,1));
  REQUIRE(CheckBitsNeededToEncodeNumber(7,3));
  REQUIRE(CheckBitsNeededToEncodeNumber(8,4));
  REQUIRE(CheckBitsNeededToEncodeNumber(255,8));
  REQUIRE(CheckBitsNeededToEncodeNumber(256,9));
  REQUIRE(CheckBitsNeededToEncodeNumber(65535,16));
  REQUIRE(CheckBitsNeededToEncodeNumber(65536,17));
}

TEST_CASE("CheckBytesNeededToEncodeNumber") {
  REQUIRE(CheckBytesNeededToEncodeNumber(0,1));
  REQUIRE(CheckBytesNeededToEncodeNumber(1,1));
  REQUIRE(CheckBytesNeededToEncodeNumber(255,1));
  REQUIRE(CheckBytesNeededToEncodeNumber(256,2));
  REQUIRE(CheckBytesNeededToEncodeNumber(65535,2));
  REQUIRE(CheckBytesNeededToEncodeNumber(65536,3));
}
