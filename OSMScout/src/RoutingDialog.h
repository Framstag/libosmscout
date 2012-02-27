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
  double  distance;
  double  distanceDelta;
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
  void PrepareRouteStep(const std::list<osmscout::RouteDescription::Node>::const_iterator& prevNode,
                        const std::list<osmscout::RouteDescription::Node>::const_iterator& node,
                        size_t lineCount,
                        RouteStep& step);
  bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node);

public:
  RoutingDialog(QWidget* parentWindow);
  ~RoutingDialog();
};

#endif
