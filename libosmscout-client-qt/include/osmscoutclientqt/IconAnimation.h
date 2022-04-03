#ifndef OSMSCOUT_CLIENT_QT_ICONANIMATION_H
#define OSMSCOUT_CLIENT_QT_ICONANIMATION_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2022  Lukáš Karas

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscout/util/Projection.h>
#include <osmscoutclientqt/IconLookup.h>

#include <QObject>
#include <QTimer>
#include <QPainter>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API IconAnimation : public QObject
{
  Q_OBJECT
private:
  static constexpr int animationDuration = 200;
  QTimer timer;

  enum class State {
    FadeIn,
    Full,
    FadeOut
  };

  struct Animation {
    MapIcon icon;
    double size;
    double startSize;
    State state;
    QElapsedTimer duration;

    double MaxSize() const
    {
      return icon.image.width();
    }

    double MinSize() const
    {
      return icon.dimensions.width();
    }
  };

  std::vector<Animation> icons;

public slots:
  void tick();

signals:
  void update();

public:
  IconAnimation();
  ~IconAnimation() override = default;

  void activate(const MapIcon &icon);
  void deactivateAll();
  void paint(QPainter *painter, const MercatorProjection &projection);
};

}

#endif // OSMSCOUT_CLIENT_QT_ICONANIMATION_H
