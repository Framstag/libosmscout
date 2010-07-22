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

#include <iostream>

#include <QtGui>

#include "DBThread.h"
#include "MapWidget.h"

class MainWindow : public QMainWindow
{
private:
  MapWidget * map;

public:
  MainWindow();
  ~MainWindow();
};

MainWindow::MainWindow()
 : map(new MapWidget())
{
  setCentralWidget(map);
}

MainWindow::~MainWindow()
{
}

int main(int argc, char* argv[])
{
  QApplication app(argc,argv);
  MainWindow   window;
  int          result;

  qRegisterMetaType<RenderMapRequest>();

  std::cout << "Starting DB thread..." << std::endl;
  dbThread.start();

  std::cout << "Opening main window..." << std::endl;
  window.setWindowTitle("OSMScout");
  window.resize(800,480);
  window.show();

  std::cout << "Entering main event loop" << std::endl;

  result=app.exec();

  std::cout << "Main event loop finished." << std::endl;

  std::cout << "Quitting DB thread..." << std::endl;

  dbThread.quit();

  std::cout << "Bye!" << std::endl;

  return result;
}
