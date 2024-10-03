#include <iostream>

#include <osmscout/util/Number.h>

#include <catch2/catch_test_macros.hpp>

bool CheckEncode(uint64_t value,
                 const char* expected, size_t expectedLength)
{
  std::array<char,10> buffer;
  size_t              bytes;

  bytes=osmscout::EncodeNumber(value,buffer);

  if (expectedLength!=bytes) {
    std::cerr << "Encoding of '" << value << "' returned wrong length - Expected " << expectedLength << " actual " << bytes << std::endl;
    return false;
  }

  for (size_t i=0; i<bytes; i++) {
    if (expected[i]!=buffer[i]) {
      std::cerr << "Encoding of '" << value << "' returned wrong data at offset " << i << " - expected " << std::hex << (unsigned short int)expected[i] << " actual " << std::hex << (unsigned short int)buffer[i] << std::endl;

      for (size_t i=0; i<bytes; i++) {
        std::cerr << std::hex << (unsigned int)buffer[i] << " ";
      }
      std::cerr << std::endl;

      return false;
    }
  }

  return true;
}

bool CheckDecode(const char* buffer, uint64_t expected, size_t bytesExpected)
{
  uint64_t value;
  size_t   bytes;

  bytes=osmscout::DecodeNumber(buffer,value);

  if (value!=expected) {
    std::cerr << "Error in decoding: expected " << expected << " actual " << value << std::endl;
    return false;
  }

  if (bytes!=bytesExpected) {
    std::cerr << "Error in decoding: expected " << bytesExpected << "bytes read, actual " << bytes << std::endl;
    return false;
  }

  return true;
}

TEST_CASE("Check encode")
{
    REQUIRE(CheckEncode(0, "\0", 1));
    REQUIRE(CheckEncode(1, "\x01", 1));
    REQUIRE(CheckEncode(127, "\x7f", 1));
    REQUIRE(CheckEncode(128, "\x80\x01", 2));
    REQUIRE(CheckEncode(213, "\xd5\x01", 2));
    REQUIRE(CheckEncode(255, "\xff\x01", 2));
    REQUIRE(CheckEncode(256, "\x80\x02", 2));
    REQUIRE(CheckEncode(65535, "\xff\xff\x03", 3));
    REQUIRE(CheckEncode(3622479373539965697, "\x81\xf6\x9d\xd0\xc2\xb9\xe8\xa2\x32", 9));
    REQUIRE(CheckEncode(3627142814677078785, "\x81\xa6\xc0\xd3\xc2\xe5\x8c\xab\x32", 9));
}

TEST_CASE("Check decode")
{
    REQUIRE(CheckDecode("\0", 0, 1));
    REQUIRE(CheckDecode("\x01", 1, 1));
    REQUIRE(CheckDecode("\x7f", 127, 1));
    REQUIRE(CheckDecode("\x80\x01", 128, 2));
    REQUIRE(CheckDecode("\xd5\x01", 213, 2));
    REQUIRE(CheckDecode("\xff\x01", 255, 2));
    REQUIRE(CheckDecode("\x80\x02", 256, 2));
    REQUIRE(CheckDecode("\xff\xff\x03", 65535, 3));
    REQUIRE(CheckDecode("\x81\xf6\x9d\xd0\xc2\xb9\xe8\xa2\x32", 3622479373539965697, 9));
    REQUIRE(CheckDecode("\x81\xa6\xc0\xd3\xc2\xe5\x8c\xab\x32", 3627142814677078785, 9));
}
