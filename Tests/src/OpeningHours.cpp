#include <osmscout/util/OpeningHours.h>

#include <TestMain.h>

TEST_CASE("Parse garbage")
{
  REQUIRE(osmscout::OpeningHours::Parse("garbage") == std::nullopt);
}

TEST_CASE("Parse 24/7")
{
  auto oh=osmscout::OpeningHours::Parse("24/7");
  REQUIRE(oh != std::nullopt);

  REQUIRE(oh->GetRules().size()==7);
  REQUIRE(oh->GetRules()[0].intervals.size()==1);

  REQUIRE(oh->GetRules()[0].intervals[0].from.hour==0);
  REQUIRE(oh->GetRules()[0].intervals[0].from.minute==0);
  REQUIRE(oh->GetRules()[0].intervals[0].to.hour==24);
  REQUIRE(oh->GetRules()[0].intervals[0].to.minute==0);
}

TEST_CASE("Parse simple opening hours")
{
  auto oh=osmscout::OpeningHours::Parse("Mo-Fr 08:00-12:00,13:00-17:30");
  REQUIRE(oh != std::nullopt);

  REQUIRE(oh->GetRules().size()==5);
  REQUIRE(oh->GetRules()[0].day==osmscout::OpeningHours::WeekDay::Monday);
  REQUIRE(oh->GetRules()[1].day==osmscout::OpeningHours::WeekDay::Tuesday);
  REQUIRE(oh->GetRules()[2].day==osmscout::OpeningHours::WeekDay::Wednesday);
  REQUIRE(oh->GetRules()[3].day==osmscout::OpeningHours::WeekDay::Thursday);
  REQUIRE(oh->GetRules()[4].day==osmscout::OpeningHours::WeekDay::Friday);

  REQUIRE(oh->GetRules()[0].intervals.size()==2);

  REQUIRE(oh->GetRules()[0].intervals[0].from.hour==8);
  REQUIRE(oh->GetRules()[0].intervals[0].from.minute==0);
  REQUIRE(oh->GetRules()[0].intervals[0].to.hour==12);
  REQUIRE(oh->GetRules()[0].intervals[0].to.minute==0);

  REQUIRE(oh->GetRules()[0].intervals[1].from.hour==13);
  REQUIRE(oh->GetRules()[0].intervals[1].from.minute==0);
  REQUIRE(oh->GetRules()[0].intervals[1].to.hour==17);
  REQUIRE(oh->GetRules()[0].intervals[1].to.minute==30);
}

TEST_CASE("Parse multiple rules")
{
  auto oh=osmscout::OpeningHours::Parse("Mo-Fr 08:00-18:00; Sa 08:00-12:00");
  REQUIRE(oh != std::nullopt);

  REQUIRE(oh->GetRules().size()==6);
}

TEST_CASE("Parse multiple days")
{
  auto oh = osmscout::OpeningHours::Parse("Mo,We 08:00-12:00");
  REQUIRE(oh != std::nullopt);

  REQUIRE(oh->GetRules().size() == 2);
  REQUIRE(oh->GetRules()[0].day==osmscout::OpeningHours::WeekDay::Monday);
  REQUIRE(oh->GetRules()[1].day==osmscout::OpeningHours::WeekDay::Wednesday);
}

TEST_CASE("Public holidays as weekend")
{
  auto oh = osmscout::OpeningHours::Parse("Mo-Fr 08:00-18:00; Sa-Su,PH 08:00-12:00");
  REQUIRE(oh != std::nullopt);
  REQUIRE(oh->GetRules().size() == 8);
}

TEST_CASE("Closed at public holidays")
{
  auto oh = osmscout::OpeningHours::Parse("Mo-Fr 08:00-18:00; PH off");
  REQUIRE(oh != std::nullopt);
  REQUIRE(oh->GetRules().size() == 6);
  REQUIRE(oh->GetRules()[5].day==osmscout::OpeningHours::WeekDay::PublicHoliday);
  REQUIRE(oh->GetRules()[5].intervals.empty());
}
