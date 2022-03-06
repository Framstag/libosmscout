#ifndef LIBOSMSCOUT_MAPPROJECTION_H
#define LIBOSMSCOUT_MAPPROJECTION_H

/*
  This source is part of the libosmscout-map-opengl library
  Copyright (C) 2022  Lukas Karas

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

#include <osmscout/GeoCoord.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Projection.h>

#include <osmscoutmapopengl/MapOpenGLImportExport.h>

namespace osmscout {

  /**
   * OpenGL renderer map projection
   */
  struct OSMSCOUT_MAP_OPENGL_API MapProjection
  {
    osmscout::GeoCoord center;
    osmscout::Magnification magnification;
    double dpi;
    int width;
    int height;

    MercatorProjection Mercator() const
    {
      MercatorProjection mercator;
      mercator.Set(center, magnification, dpi, width, height);
      return mercator;
    }
  };
}

#endif //LIBOSMSCOUT_MAPPROJECTION_H
