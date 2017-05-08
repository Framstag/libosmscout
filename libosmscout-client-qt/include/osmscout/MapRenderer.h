/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukas Karas

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

#ifndef MAPRENDERER_H
#define MAPRENDERER_H

#include <QObject>
#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

class OSMSCOUT_CLIENT_QT_API MapRenderer : QObject {
  Q_OBJECT

signals:
  void Redraw();

public slots:
  virtual void InvalidateVisualCache() = 0;

protected:
  MapRenderer(SettingsRef settings,
              DBThreadRef dbThread);

public:
  virtual ~MapRenderer();
};

#endif /* MAPRENDERER_H */

