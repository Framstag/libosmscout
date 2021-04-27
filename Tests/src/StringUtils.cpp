#include <iostream>

#include <osmscout/util/String.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Split empty string")
{
  auto elements = osmscout::SplitString("", ";");
  REQUIRE(elements.empty());
}

TEST_CASE("Split string by simple separator")
{
  auto elements=osmscout::SplitString("asphalt;ground;gravel", ";");
  REQUIRE(elements.size() == 3);
  auto it=elements.begin();
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}

TEST_CASE("Separator on the end dont produce empty element")
{
  auto elements=osmscout::SplitString(";asphalt;ground;gravel;", ";");
  REQUIRE(elements.size() == 4);
  auto it=elements.begin();
  REQUIRE(*(it++) == "");
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}

TEST_CASE("Split string by multi-character separator")
{
  auto elements=osmscout::SplitString("asphalt <> ground <> gravel", " <> ");
  REQUIRE(elements.size() == 3);
  auto it=elements.begin();
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}

TEST_CASE("Transliterate diacritics")
{
  try {
    std::locale::global(std::locale("C"));
    std::cout << "C locale activated" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }

  auto transformed=osmscout::UTF8Transliterate("áéíýóúůďťňěščřžüöÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖßæÆ");

  REQUIRE(transformed == "aeiyouudtnescrzuoAEIYOUUDTNESCRZUOssaeAE");
}

TEST_CASE("Transliterate untranslated")
{
  auto transformed=osmscout::UTF8Transliterate(
        "áéíýóúůďťňěščřžüö Դուք չհասկացաք իմ պարզությունը ÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖßæÆ");

  REQUIRE(transformed ==
          "aeiyouudtnescrzuo Դուք չհասկացաք իմ պարզությունը AEIYOUUDTNESCRZUOssaeAE");
}

TEST_CASE("Uppercase 1")
{
  auto transformed=osmscout::UTF8StringToUpper("áéíýóúůďťňěščřžüöÿÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖŸß");

  REQUIRE(transformed == "ÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖŸÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖŸß");
}

TEST_CASE("Lowercase 1")
{
  auto transformed=osmscout::UTF8StringToLower("áéíýóúůďťňěščřžüöÿÁÉÍÝÓÚŮĎŤŇĚŠČŘŽÜÖŸß");

  REQUIRE(transformed == "áéíýóúůďťňěščřžüöÿáéíýóúůďťňěščřžüöÿß");
}

TEST_CASE("Uppercase 2")
{
  auto transformed=osmscout::UTF8StringToUpper(
        "Δεν καταλάβατε τίποτα από την απλότητά μου, τίποτα, ω φτωχό παιδί "
        "μου! Και είναι με ένα παλιό φρύδι, ενοχλημένο που φεύγεις πριν.");

  REQUIRE(transformed ==
          "ΔΕΝ ΚΑΤΑΛΆΒΑΤΕ ΤΊΠΟΤΑ ΑΠΌ ΤΗΝ ΑΠΛΌΤΗΤΆ ΜΟΥ, ΤΊΠΟΤΑ, Ω ΦΤΩΧΌ ΠΑΙΔΊ "
          "ΜΟΥ! ΚΑΙ ΕΊΝΑΙ ΜΕ ΈΝΑ ΠΑΛΙΌ ΦΡΎΔΙ, ΕΝΟΧΛΗΜΈΝΟ ΠΟΥ ΦΕΎΓΕΙΣ ΠΡΙΝ.");
}

TEST_CASE("Lowercase 2")
{
  auto transformed=osmscout::UTF8StringToLower(
        "ΔΕΝ ΚΑΤΑΛΆΒΑΤΕ ΤΊΠΟΤΑ ΑΠΌ ΤΗΝ ΑΠΛΌΤΗΤΆ ΜΟΥ, ΤΊΠΟΤΑ, Ω ΦΤΩΧΌ ΠΑΙΔΊ "
        "ΜΟΥ! ΚΑΙ ΕΊΝΑΙ ΜΕ ΈΝΑ ΠΑΛΙΌ ΦΡΎΔΙ, ΕΝΟΧΛΗΜΈΝΟ ΠΟΥ ΦΕΎΓΕΙΣ ΠΡΙΝ.");

  REQUIRE(transformed ==
          "δεν καταλάβατε τίποτα από την απλότητά μου, τίποτα, ω φτωχό παιδί "
          "μου! και είναι με ένα παλιό φρύδι, ενοχλημένο που φεύγεισ πριν.");
}

TEST_CASE("Uppercase 3")
{
  auto transformed=osmscout::UTF8StringToUpper(
        "لم تفهم شيئًا من بساطتي ، لا شيء ، يا ولدي المسكين! ومع جبين لا معنى له ، منزعج أن تهرب من قبل.");

  REQUIRE(transformed ==
          "لم تفهم شيئًا من بساطتي ، لا شيء ، يا ولدي المسكين! ومع جبين لا معنى له ، منزعج أن تهرب من قبل.");
}

TEST_CASE("Lowercase 3")
{
  auto transformed=osmscout::UTF8StringToLower(
        "لم تفهم شيئًا من بساطتي ، لا شيء ، يا ولدي المسكين! ومع جبين لا معنى له ، منزعج أن تهرب من قبل.");

  REQUIRE(transformed ==
          "لم تفهم شيئًا من بساطتي ، لا شيء ، يا ولدي المسكين! ومع جبين لا معنى له ، منزعج أن تهرب من قبل.");
}

TEST_CASE("Normalize for lookup")
{
  auto transformed=osmscout::UTF8NormForLookup(
          "Baker \t \u00A0 \u2007 \u202F Street");

  REQUIRE(transformed ==
          "baker street");
}

TEST_CASE("Parse illegal UTF8 sequence")
{
  auto transformed=osmscout::UTF8Transliterate(u8"\xef\xbb\xbf\x2f\xc0\xae\x2e\x2f");

  REQUIRE(transformed == u8"\xef\xbb\xbf\x2f\x2e\x2f");
}

TEST_CASE("Local aware number to string")
{
  osmscout::Locale locale;
  locale.SetThousandsSeparator(" ");
  REQUIRE(osmscout::NumberToString(1002030, locale) == "1 002 030");
  REQUIRE(osmscout::NumberToString(-1002030, locale) == "-1 002 030");
}
