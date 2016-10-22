/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <QString>
#include <QDebug>

#include "osmscout/OSMTile.h"
#include "osmscout/util/GeoBox.h"

osmscout::GeoBox OSMTile::tileBoundingBox(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    double lon = tilex2lon(xtile, zoomLevel);
    double lonRes = 360.0 / worldRes(zoomLevel);
    double lat = tiley2lat(ytile, zoomLevel);
    double lat2 = tiley2lat(ytile + 1, zoomLevel);

    /*
    qDebug() << "Tile " << zoomLevel << " " << xtile << " " << ytile << " box: " << 
            QString::fromStdString(b.GetDisplayText());
    */
    
    return osmscout::GeoBox(
            osmscout::GeoCoord(lat, lon), 
            osmscout::GeoCoord(lat2, lon + lonRes)
            );
}

osmscout::GeoCoord OSMTile::tileRelativeCoord(uint32_t zoomLevel, double x, double y)
{
    double n = M_PI - 2.0 * M_PI * y / (double)worldRes(zoomLevel);
    double lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    
    double lon = x / (double)worldRes(zoomLevel) * 360.0 - 180;
    
    return osmscout::GeoCoord(lat, lon);
}

osmscout::GeoCoord OSMTile::tileVisualCenter(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    return OSMTile::tileRelativeCoord(zoomLevel, (double)xtile + 0.5, (double)ytile + 0.5);
}
    