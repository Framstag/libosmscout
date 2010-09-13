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

MainWindow::MainWindow()
 : map(new MapWidget())
{
  QMenu *menu;

  menu=menuBar()->addMenu("&File");
  menu->addAction("&Quit",this,SLOT(close()),QKeySequence("Ctrl+Q"));

  menu=menuBar()->addMenu("&Search");
  searchLocationAction=menu->addAction("Search &location",this,SLOT(SearchLocation()),QKeySequence("Ctrl+F"));
  routingAction=menu->addAction("&Routing",this,SLOT(Routing()),QKeySequence("Ctrl+R"));

  searchLocationAction->setEnabled(false);
  routingAction->setEnabled(false);

  setCentralWidget(map);

  connect(&dbThread,SIGNAL(InitialisationFinished(DatabaseLoadedResponse)),
          this,SLOT(InitialisationFinished(DatabaseLoadedResponse)));
}

MainWindow::~MainWindow()
{
  // no code
}

void MainWindow::InitialisationFinished(const DatabaseLoadedResponse& response)
{
  searchLocationAction->setEnabled(true);
  routingAction->setEnabled(true);
}

void MainWindow::SearchLocation()
{
  SearchLocationDialog dialog(this);

  dialog.exec();

  if (dialog.result()==QDialog::Accepted) {
    osmscout::Location location;

    location=dialog.GetLocationResult();

    map->ShowReference(location.references.front(),osmscout::magVeryClose);
  }
}

void MainWindow::Routing()
{
  RoutingDialog dialog(this);

  dialog.exec();
}

#include "moc_MainWindow.cpp"
