#ifndef OSMSCOUT_CLIENT_QT_OPENING_HOURS_MODEL
#define OSMSCOUT_CLIENT_QT_OPENING_HOURS_MODEL

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscout/util/OpeningHours.h>

#include <QObject>
#include <QDateTime>
#include <QAbstractListModel>
#include <QLocale>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * QML Component for parsing OSM opening hours.
 */
class OSMSCOUT_CLIENT_QT_API OpeningHoursModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString openingHours READ getOpeningHours WRITE setOpeningHours NOTIFY updated)
  Q_PROPERTY(QStringList today READ getToday NOTIFY updated)

signals:
  void updated();
  void parseError();

public:
  enum Roles {
    DayRole = Qt::UserRole,
    ShortDayRole = Qt::UserRole+1,
    TimeIntervalsRole = Qt::UserRole+2,
    IsTodayRole = Qt::UserRole+3,
  };
  Q_ENUM(Roles)

public:
  OpeningHoursModel() = default;
  OpeningHoursModel(const OpeningHoursModel &) = delete;
  OpeningHoursModel(OpeningHoursModel&&) = delete;

  ~OpeningHoursModel() override = default;

  OpeningHoursModel& operator=(const OpeningHoursModel&) = delete;
  OpeningHoursModel& operator=(OpeningHoursModel&&) = delete;

  Q_INVOKABLE int inline rowCount(const QModelIndex &parent = QModelIndex()) const override
  {
    Q_UNUSED(parent);
    return model.size();
  };

  Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;

  QStringList getToday() const;

  QString getOpeningHours() const
  {
    return rawOpeningHours;
  }

  void setOpeningHours(const QString &openingHours);

private:
  QStringList intervalStrings(const std::vector<OpeningHours::TimeInterval> &intervals) const;
  QString dayName(OpeningHours::WeekDay weekDay) const;
  QString shortDayName(OpeningHours::WeekDay weekDay) const;

private:
  QString rawOpeningHours;
  std::vector<OpeningHours::Rule> model;
  QLocale locale;
};
}

#endif /* OSMSCOUT_CLIENT_QT_OPENING_HOURS_MODEL */
