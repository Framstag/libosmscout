#ifndef LIBOSMSCOUT_MAPOVERLAY_H
#define LIBOSMSCOUT_MAPOVERLAY_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukáš Karas

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

#include <osmscout/InputHandler.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QQuickPaintedItem>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapOverlay : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(QObject *view READ GetView WRITE SetMapView)

protected:
  MapView          *view;

public slots:
  void changeView(const MapView &view);
  void redraw();

public:
  MapOverlay(QQuickItem* parent = 0);
  virtual ~MapOverlay();

  inline void SetMapView(QObject *o)
  {
    MapView *updated = dynamic_cast<MapView*>(o);
    if (updated == NULL){
      qWarning() << "Failed to cast " << o << " to MapView*.";
      return;
    }

    bool changed = *view != *updated;
    if (changed){
      changeView(*updated);
    }
  }

  inline MapView* GetView() const
  {
    return view; // We should be owner, parent is set http://doc.qt.io/qt-5/qqmlengine.html#objectOwnership
  }
};

#endif //LIBOSMSCOUT_MAPOVERLAY_H
