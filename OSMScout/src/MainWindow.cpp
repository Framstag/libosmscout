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

#include "MainWindow.h"

#include <QMenuBar>

#include "SearchLocationDialog.h"
#include "RoutingDialog.h"
#include "SettingsDialog.h"

MainWindow::MainWindow(Settings* settings,
                       DBThread *dbThread)
 : settings(settings),
   dbThread(dbThread),
   map(new MapWidget(NULL,
                     dbThread))
{
  QMenu *menu;

  menu=menuBar()->addMenu("&File");
  menu->addAction("&Quit",this,SLOT(close()),QKeySequence("Ctrl+Q"));

  menu=menuBar()->addMenu("&Search");
  searchLocationAction=menu->addAction("Search &location",
                                       this,
                                       SLOT(OpenSearchLocationDialog()),
                                       QKeySequence("Ctrl+F"));
  routingAction=menu->addAction("&Routing",
                                this,
                                SLOT(OpenRoutingDialog()),
                                QKeySequence("Ctrl+R"));
  menu->addSeparator();
  settingsAction=menu->addAction("&Settings",
                                 this,
                                 SLOT(OpenSettingsDialog()));

  searchLocationAction->setEnabled(false);
  routingAction->setEnabled(false);
  settingsAction->setEnabled(false);

  setCentralWidget(map);

  connect(dbThread,
          SIGNAL(InitialisationFinished(DatabaseLoadedResponse)),
          this,
          SLOT(InitialisationFinished(DatabaseLoadedResponse)));
}

MainWindow::~MainWindow()
{
  // no code
}

void MainWindow::InitialisationFinished(const DatabaseLoadedResponse& response)
{
  searchLocationAction->setEnabled(true);
  routingAction->setEnabled(true);
  settingsAction->setEnabled(true);
}

void MainWindow::OpenSearchLocationDialog()
{
  SearchLocationDialog dialog(this,
                              dbThread);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location location;

    location=dialog.GetLocationResult();

    map->ShowReference(location.references.front(),
                       osmscout::Magnification::magVeryClose);
  }
}

void MainWindow::OpenRoutingDialog()
{
  RoutingDialog dialog(this,
                       dbThread);

  dialog.exec();
}

void MainWindow::OpenSettingsDialog()
{
  SettingsDialog dialog(this,
                        settings);

  dialog.exec();
}

#include "moc_MainWindow.cpp"
