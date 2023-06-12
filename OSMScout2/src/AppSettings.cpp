/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukas Karas

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

#include <QDebug>

#include <osmscoutclientqt/InputHandler.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <AppSettings.h>

using namespace osmscout;

AppSettings::AppSettings(): view(nullptr)
{
}

MapView *AppSettings::GetMapView()
{
  if (view == nullptr){
    double lat   = settings.value("settings/map/lat",   0).toDouble();
    double lon   = settings.value("settings/map/lon",   0).toDouble();
    double angle = settings.value("settings/map/angle", 0).toDouble();
    double mag   = settings.value("settings/map/mag",   pow(2.0,osmscout::Magnification::magContinent.Get())).toDouble();
    view = new MapView(this,
              osmscout::GeoCoord(lat, lon),
              Bearing::Radians(angle),
              std::max(osmscout::Magnification(osmscout::Magnification::magWorld),
                       std::min(osmscout::Magnification(mag),
                                osmscout::Magnification(osmscout::Magnification::magHouse))),
              OSMScoutQt::GetInstance().GetSettings()->GetMapDPI()
              );
  }
  return view;
}

void AppSettings::SetMapView(QObject *o)
{
  MapView *updated = dynamic_cast<MapView*>(o);
  if (updated == nullptr){
    qWarning() << "Failed to cast " << o << " to MapView*.";
    return;
  }
  bool changed = false;
  if (view == nullptr){
    view = new MapView(this,
                       updated->center,
                       updated->angle,
                       updated->magnification,
                       updated->mapDpi);
    changed = true;
  }else{
    changed = *view != *updated;
    if (changed){
        *view = *updated;
    }
  }
  if (changed){
    settings.setValue("settings/map/lat", view->GetLat());
    settings.setValue("settings/map/lon", view->GetLon());
    settings.setValue("settings/map/angle", view->GetAngle());
    settings.setValue("settings/map/mag", view->GetMag());
    emit MapViewChanged(view);
  }
}
