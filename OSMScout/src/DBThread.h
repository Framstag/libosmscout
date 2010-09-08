#ifndef DBTHREAD_H
#define DBTHREAD_H

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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>

#include "config.h"

#if defined(HAVE_LIB_QTOPENGL)
 #include <QGLPixelBuffer>
#endif

#include <osmscout/Database.h>
#include <osmscout/MapPainterQt.h>

struct RenderMapRequest
{
  double lon;
  double lat;
  double magnification;
  size_t width;
  size_t height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

struct DatabaseLoadedResponse
{
  double minLat;
  double minLon;
  double maxLat;
  double maxLon;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

class DBThread : public QThread
{
  Q_OBJECT

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void HandleMapRenderingResult();

public slots:
  void TriggerMapRendering(const RenderMapRequest& request);

private:
  QMutex                    mutex;
  osmscout::Database        database;
  osmscout::StyleConfig     *styleConfig;
  osmscout::MapData         data;
  osmscout::MapPainterQt    painter;

  bool                      finish;

  QPixmap                   *currentPixmap;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer            *currentGLPixmap;
#endif
  double                    currentLon,currentLat;
  double                    currentMagnification;

  QPixmap                   *finishedPixmap;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer            *finishedGLPixmap;
#endif
  double                    finishedLon,finishedLat;
  double                    finishedMagnification;

public:
  DBThread();

  void run();

  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);
};

extern DBThread dbThread;

#endif
