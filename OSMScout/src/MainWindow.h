#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

#include <QMainWindow>

#include "Settings.h"

#include "MapWidget.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public slots:
  void OpenSearchLocationDialog();
  void OpenRoutingDialog();
  void OpenSettingsDialog();
  void InitialisationFinished(const DatabaseLoadedResponse& response);

private:
  SettingsRef settings;
  DBThread    *dbThread;

  MapWidget   *map;
  QAction     *searchLocationAction;
  QAction     *routingAction;
  QAction     *settingsAction;

public:
  MainWindow(const SettingsRef& settings,
             DBThread *dbThread);
  ~MainWindow();


};

#endif
