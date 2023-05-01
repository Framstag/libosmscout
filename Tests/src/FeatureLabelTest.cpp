
#include <osmscout/feature/EleFeature.h>

#include <TestMain.h>

using namespace osmscout;

TEST_CASE("Elevation label formatting")
{
  EleFeatureValue ele;
  ele.SetEle(1000);

  Locale locale; // C locale
  REQUIRE(ele.GetLabel(locale, EleFeature::IN_METER_LABEL_INDEX) == "1000\xE2\x80\xAFm");

  locale.SetThousandsSeparator(",");
  REQUIRE(ele.GetLabel(locale, EleFeature::IN_METER_LABEL_INDEX) == "1,000\xE2\x80\xAFm");

  REQUIRE(ele.GetLabel(locale, EleFeature::IN_LOCALE_UNIT_LABEL_INDEX) == "1,000\xE2\x80\xAFm");

  locale.SetUnitsSeparator(" ");
  REQUIRE(ele.GetLabel(locale, EleFeature::IN_FEET_LABEL_INDEX) == "3,281 ft");

  locale.SetUnitsSeparator("");
  REQUIRE(ele.GetLabel(locale, EleFeature::IN_FEET_LABEL_INDEX) == "3,281ft");

  locale.SetDistanceUnits(DistanceUnitSystem::Imperial);
  REQUIRE(ele.GetLabel(locale, EleFeature::IN_LOCALE_UNIT_LABEL_INDEX) == "3,281ft");
}
