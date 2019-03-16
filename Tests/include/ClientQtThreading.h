/*
  ClientQtThreading - a test program for libosmscout
  Copyright (C) 2019  Lukas Karas

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

#ifndef LIBOSMSCOUT_CLIENTQTTHREADING_H
#define LIBOSMSCOUT_CLIENTQTTHREADING_H

#include <osmscout/StyleModule.h>

#include <QObject>
#include <QStringList>
#include <QTimer>

class ThreadingTest: public QObject
{
  Q_OBJECT

public slots:
  void test();

  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles);

signals:
  void loadStyleRequested(QString,std::unordered_map<std::string,bool>);

public:
  ThreadingTest(const QList<QFileInfo> &stylesheets,
                const osmscout::GeoCoord &renderingCenter,
                const size_t &magLevel);

  ~ThreadingTest();

  inline bool isFailed() const
  {
    return failure;
  }

private:
  QTimer timer;
  QList<QFileInfo> stylesheets;
  osmscout::GeoCoord renderingCenter;
  size_t magLevel;
  osmscout::StyleModule* styleModule;
  osmscout::DBThreadRef dbThread;
  osmscout::DBLoadJob *loadJob{nullptr};
  size_t stylesheetCtn{0};
  bool failure{false};
  QMap<QString, size_t> objectCountPerStylesheet;
};


#endif //LIBOSMSCOUT_CLIENTQTTHREADING_H
