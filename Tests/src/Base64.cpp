#include <osmscout/util/Base64.h>

#include <TestMain.h>

const char TestString[] = "Lorem ipsum dolor sit amet, "
                                    "consectetuer adipiscing elit. "
                                    "Nunc auctor. Maecenas ipsum velit...";

TEST_CASE("Base64 encode") {
  auto stringBytes = std::vector<char>(TestString, TestString + sizeof(TestString));
  REQUIRE(osmscout::Base64Encode(stringBytes) ==
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVlciBhZGlwaXNjaW5nIGVsaXQu"
      "IE51bmMgYXVjdG9yLiBNYWVjZW5hcyBpcHN1bSB2ZWxpdC4uLgA=");
}

TEST_CASE("Base64 decode") {
  auto stringBytes = std::vector<char>(TestString, TestString + sizeof(TestString));
  REQUIRE(osmscout::Base64Decode(
      "\t\t"
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVlciBhZGlwaXNjaW5nIGVsaXQu\n"
      "IE51bmMgYXVjdG9yLiBNYWVjZW5hcyBpcHN1bSB2ZWxpdC4uLgA=  ") == stringBytes);
}

TEST_CASE("Base64 processing binary data") {
  std::vector<char> binaryData = std::vector<char>(TestString, TestString + sizeof(TestString));
  std::string encoded = osmscout::Base64Encode(binaryData);
  //std::cout << encoded << std::endl;
  std::vector<char> decoded = osmscout::Base64Decode(encoded);

  REQUIRE(decoded == binaryData);
}
