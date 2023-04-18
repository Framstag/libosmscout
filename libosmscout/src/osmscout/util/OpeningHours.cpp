/*
  This source is part of the libosmscout library
  Copyright (C) 2023 Lukáš Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/util/OpeningHours.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Logger.h>

#include <array>

namespace osmscout {

OpeningHours::OpeningHours(std::vector<Rule> rules):
  rules(rules)
{
}

namespace { // anonymous namespace

std::optional<OpeningHours::WeekDay> ParseDay(const std::string &dayStr, bool holidays)
{
  if (dayStr=="Mo") {
    return OpeningHours::WeekDay::Monday;
  }
  if (dayStr=="Tu") {
    return OpeningHours::WeekDay::Tuesday;
  }
  if (dayStr=="We") {
    return OpeningHours::WeekDay::Wednesday;
  }
  if (dayStr=="Th") {
    return OpeningHours::WeekDay::Thursday;
  }
  if (dayStr=="Fr") {
    return OpeningHours::WeekDay::Friday;
  }
  if (dayStr=="Sa") {
    return OpeningHours::WeekDay::Saturday;
  }
  if (dayStr=="Su") {
    return OpeningHours::WeekDay::Sunday;
  }
  if (dayStr=="PH" && holidays) {
    return OpeningHours::WeekDay::PublicHoliday;
  }
  if (dayStr=="SH" && holidays) {
    return OpeningHours::WeekDay::SchoolHoliday;
  }

  log.Warn() << "Unknown opening hours day: " << dayStr;
  return std::nullopt;
}

// examples:
// Mo-Su
// Mo,Tu
// Sa-Su,PH
std::optional<std::vector<OpeningHours::WeekDay>> ParseDayDescription(const std::string &dayDescription)
{
  std::vector<OpeningHours::WeekDay> result;

  // dayDescription is day set (Mo,Tu or Sa-Su,PH)
  auto days=SplitString(dayDescription, ",");
  for (auto const &dayStr:days) {

    if (dayStr.find('-') != std::string::npos) {
      // dayDescription is day interval (Mo-Su)
      auto daysIntervalParts = SplitString(dayStr, "-", 2);
      if (daysIntervalParts.size() != 2) {
        log.Warn() << "Cannot parse day description: " << dayDescription;
        return std::nullopt;
      }
      auto from=ParseDay(daysIntervalParts.front(), false);
      auto to=ParseDay(daysIntervalParts.back(), false);
      if (!from || !to) {
        return std::nullopt;
      }
      for (int day=int(from.value()); day<=int(to.value()); day++) {
        result.push_back(OpeningHours::WeekDay(day));
      }
    } else {
      // single day
      auto day = ParseDay(dayStr, true);
      if (!day) {
        return std::nullopt;
      }
      result.push_back(*day);
    }
  }

  return result;
}

// example: 08:00
std::optional<OpeningHours::DayTime> ParseTime(const std::string &timeStr)
{
  auto timesStr=SplitString(timeStr, ":", 2);
  if (timesStr.size()!=2) {
    log.Warn() << "Cannot parse time: " << timeStr;
    return std::nullopt;
  }
  OpeningHours::DayTime result;
  if (!StringToNumberUnsigned(timesStr.front(), result.hour)) {
    log.Warn() << "Cannot parse time: " << timeStr;
    return std::nullopt;
  }
  if (!StringToNumberUnsigned(timesStr.back(), result.minute)) {
    log.Warn() << "Cannot parse time: " << timeStr;
    return std::nullopt;
  }
  if (result.hour>48 || result.minute>59) {
    log.Warn() << "Cannot parse time: " << timeStr;
    return std::nullopt;
  }
  return result;
}

// example: 08:00-12:00
std::optional<OpeningHours::TimeInterval> ParseTimeRange(const std::string &rangeStr)
{
  auto timesStr=SplitString(rangeStr, "-", 2);
  if (timesStr.size()!=2) {
    log.Warn() << "Cannot parse time interval: " << rangeStr;
    return std::nullopt;
  }
  auto from=ParseTime(timesStr.front());
  if (!from) {
    return std::nullopt;
  }
  auto to=ParseTime(timesStr.back());
  if (!to) {
    return std::nullopt;
  }
  return OpeningHours::TimeInterval{*from, *to};
}

// example: 08:00-12:00,13:00-17:30
std::optional<std::vector<OpeningHours::TimeInterval>> ParseTimeDescription(const std::string &timeDescription) {
  std::vector<OpeningHours::TimeInterval> result;
  if (timeDescription=="off") {
    return result;
  }
  auto rangesStr=SplitString(timeDescription, ",");
  for (const auto &rangeStr:rangesStr) {
    auto range=ParseTimeRange(rangeStr);
    if (!range) {
      return std::nullopt;
    }
    result.push_back(*range);
  }
  return result;
}
}

std::optional<OpeningHours> OpeningHours::Parse(const std::string &str)
{
  constexpr int LastDayValue = int(WeekDay::SchoolHoliday);

  if (str=="24/7") {
    std::vector<Rule> rules;
    std::vector<TimeInterval> wholeDay{{DayTime{0, 0}, DayTime{24, 0}}};
    for (int day=int(WeekDay::Monday); day<=int(WeekDay::Sunday); day++) {
      rules.push_back(Rule{WeekDay(day),wholeDay});
    }
    return std::make_optional<OpeningHours>(rules);
  }

  // to make sure that we have one rule per day (the last one), use array indexed by day
  std::array<std::optional<std::vector<TimeInterval>>, LastDayValue+1> rules;
  auto rulesStr = SplitString(str, ";");
  for (std::string &ruleStr:rulesStr){
    ruleStr=Trim(ruleStr);
    auto ruleSplit=SplitString(ruleStr, " ", 2);
    if (ruleSplit.size()!=2) {
      log.Warn() << "Cannot parse opening hours rule: " << ruleStr;
      return std::nullopt;
    }
    auto days=ParseDayDescription(ruleSplit.front());
    if (!days) {
      return std::nullopt;
    }
    auto timeIntervals=ParseTimeDescription(ruleSplit.back());
    if (!timeIntervals) {
      return std::nullopt;
    }
    for (const auto &day: *days) {
      rules[int(day)]=timeIntervals;
    }
  }

  // copy defined day values to vector
  std::vector<Rule> rulesVec;
  for (int day=0; day<=LastDayValue; day++) {
    if (rules[day].has_value()) {
      rulesVec.push_back(Rule{WeekDay(day),rules[day].value()});
    }
  }

  return std::make_optional<OpeningHours>(rulesVec);
}

}