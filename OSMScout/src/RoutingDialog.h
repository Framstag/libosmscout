#ifndef ROUTINGDIALOG_H
#define ROUTINGDIALOG_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>

#include <osmscout/Location.h>
#include <osmscout/Route.h>

class RouteModel : public QAbstractTableModel
{
  Q_OBJECT

public:
  RouteModel(QObject *parent=0);

  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  void refresh();
};

struct RouteStep
{
  QString distance;
  QString distanceDelta;
  QString time;
  QString timeDelta;
  QString description;
};

class RoutingDialog : public QDialog
{
  Q_OBJECT

public slots:
  void SelectFrom();
  void SelectTo();
  void Route();

private:
  QLineEdit   *from;
  bool        hasStart;
  QLineEdit   *to;
  bool        hasEnd;
  QTableView  *routeView;
  RouteModel  *routeModel;
  QPushButton *routeButton;

private:
  void DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                            const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
  void DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription);
  void DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                           const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
  void DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
  void DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
  void DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                    const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
  void DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);
  void DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                    const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                    const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
  void DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

public:
  RoutingDialog(QWidget* parentWindow);
  ~RoutingDialog();
};

#endif
