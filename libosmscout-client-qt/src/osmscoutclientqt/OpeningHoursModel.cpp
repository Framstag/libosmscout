/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2023 Lukas Karas

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

#include <osmscoutclientqt/OpeningHoursModel.h>

#include <QLocale>
#include <QTextStream>

namespace osmscout {

QStringList OpeningHoursModel::intervalStrings(const std::vector<OpeningHours::TimeInterval> &intervals) const
{
  QStringList result;
  if (intervals.empty()) {
    //: example: restaurant is "closed" at Monday
    result << OpeningHoursModel::tr("closed");
  }
  auto timeString=[](const OpeningHours::DayTime &time) -> QString {
    return QString("%1:%2").arg(int(time.hour)).arg(int(time.minute), 2, 10, QLatin1Char('0'));
  };

  for (auto const interval: intervals) {
    result << timeString(interval.from) + " - " + timeString(interval.to);
  }
  return result;
}

QString OpeningHoursModel::dayName(OpeningHours::WeekDay weekDay) const
{
  if (weekDay==OpeningHours::WeekDay::PublicHoliday) {
    //: long variant
    return OpeningHoursModel::tr("Public holiday");
  }
  if (weekDay==OpeningHours::WeekDay::SchoolHoliday) {
    //: long variant
    return OpeningHoursModel::tr("School holiday");
  }
  return locale.standaloneDayName(int(weekDay)+1, QLocale::FormatType::LongFormat);
  // return calendar.standaloneWeekDayName(locale, int(weekDay)+1, QLocale::FormatType::LongFormat);
}

QString OpeningHoursModel::shortDayName(OpeningHours::WeekDay weekDay) const
{
  if (weekDay==OpeningHours::WeekDay::PublicHoliday) {
    //: short variant
    return OpeningHoursModel::tr("Public holiday");
  }
  if (weekDay==OpeningHours::WeekDay::SchoolHoliday) {
    //: short variant
    return OpeningHoursModel::tr("School holiday");
  }
  // return calendar.standaloneWeekDayName(locale, int(weekDay)+1, QLocale::FormatType::ShortFormat);
  return locale.standaloneDayName(int(weekDay)+1, QLocale::FormatType::ShortFormat);
}


QStringList OpeningHoursModel::getToday() const
{
  int dayOfTheWeek=QDate::currentDate().dayOfWeek()-1;
  if (dayOfTheWeek<0 // invalid weekday (bad locale?)
      || dayOfTheWeek>int(OpeningHours::WeekDay::Sunday) // day not in range Mo-Su, current calendar is special?
      || model.empty() // not initialized or failed to parse provided opening hours
      ) {
    return QStringList();
  }
  for (auto const &rule:model) {
    if (dayOfTheWeek==int(rule.day)) {
      return intervalStrings(rule.intervals);
    }
  }
  // we have no rule for today, so it is closed
  return intervalStrings(std::vector<OpeningHours::TimeInterval>());
}

void OpeningHoursModel::setOpeningHours(const QString &openingHours)
{
  rawOpeningHours = openingHours;
  beginResetModel();
  model.clear();
  auto result=OpeningHours::Parse(openingHours.toStdString());
  if (result) {
    bool sundayFirst=locale.firstDayOfWeek()==Qt::Sunday;
    auto dayNum=[sundayFirst](const OpeningHours::WeekDay day) -> int {
      if (sundayFirst && day==OpeningHours::WeekDay::Sunday) {
        return -1;
      }
      return int(day);
    };
    model=result->GetRules();
    std::sort(model.begin(), model.end(), [&](const OpeningHours::Rule &a, const OpeningHours::Rule &b) -> bool {
      return dayNum(a.day) < dayNum(b.day);
    });
  } else {
    emit parseError();
  }
  endResetModel();
  emit updated();
}

QVariant OpeningHoursModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= int(model.size())) {
    return QVariant();
  }
  auto const &rule = model[index.row()];
  switch (role) {
    case DayRole:
      return dayName(rule.day);
    case ShortDayRole:
      return shortDayName(rule.day);
    case TimeIntervalsRole:
      return intervalStrings(rule.intervals);
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> OpeningHoursModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[DayRole]="day";
  roles[ShortDayRole]="shortDay";
  roles[TimeIntervalsRole]="timeIntervals";

  return roles;
}

Qt::ItemFlags OpeningHoursModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}
