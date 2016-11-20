/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#ifndef OSMTILE_H
#define	OSMTILE_H

#include <cmath>

#include <osmscout/util/GeoBox.h>

/**
 * Util class with function useful for work with OSM tiles (mercator projection)
 * as defined here: http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
 * 
 * Content of OMS wiki can be distributed under terms of  
 * Creative Commons Attribution-ShareAlike 2.0 license
 * http://wiki.openstreetmap.org/wiki/Wiki_content_license
 * 
 * I am not sure if these one-line code samples can use...?
 */
static const double GRAD_TO_RAD = 2 * M_PI / 360;

class OSMTile{
public:
    static osmscout::GeoBox tileBoundingBox(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
    static osmscout::GeoCoord tileRelativeCoord(uint32_t zoomLevel, double x, double y);
    static osmscout::GeoCoord tileVisualCenter(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);

    static inline double minLat(){
        return -85.0511;
    }
    static inline double maxLat(){
        return +85.0511;
    }
    static inline double minLon(){
        return -180.0;
    }
    static inline double maxLon(){
        return 180.0;
    }
    static inline int osmTileOriginalWidth(){
        return 256;
    }
    static inline double tileDPI(){
        return 96.0;
    }
    
    static inline uint32_t lon2tilex(double lon, uint32_t z) 
    { 
        return (uint32_t)(floor((lon + 180.0) / 360.0 * (double)worldRes(z))); 
    }

    static inline uint32_t lat2tiley(double lat, uint32_t z)
    { 
        return (uint32_t)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * (double)worldRes(z))); 
    }

    static inline double tilex2lon(uint32_t x, uint32_t z) 
    {
        return (double)x / (double)worldRes(z) * 360.0 - 180;
    }

    static inline double tiley2lat(uint32_t y, uint32_t z) 
    {
        double n = M_PI - 2.0 * M_PI * y / (double)worldRes(z);
        return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    }    
    
    /**
     * world resolution on given zoom level in OSM tiles 
     */
    static inline uint32_t worldRes(uint32_t level){
        // equivalent of pow(2.0, z)
        return 1 << level;
    }
};

#endif	/* OSMTILE_H */

