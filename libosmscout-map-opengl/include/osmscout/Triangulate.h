#ifndef LIBOSMSCOUT_TRIANGULATE_H
#define LIBOSMSCOUT_TRIANGULATE_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017 Fanny Monori

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

#include <GL/glew.h>
#include <iostream>
#include <osmscout/MapPainter.h>
#include <glm/vec3.hpp>

namespace osmscout{

  class Triangulate {
  public:

    /**
     * Triangulate a polygon without hole.
     */
    static std::vector<GLfloat> TriangulatePolygon(std::vector<osmscout::Point> points);

    /**
     * Triangulate a polygon without hole.
     */
    static std::vector<GLfloat> TriangulatePolygon(std::vector<osmscout::Vertex2D> points);

    /**
     * Triangulate a polygon without hole.
     */
    static std::vector<GLfloat> TriangulatePolygon(std::vector<osmscout::GeoCoord> points);

    /**
     * Triangulate a polygon with hole.
     */
    static std::vector<GLfloat> TriangulateWithHoles(std::vector<std::vector<osmscout::Point>> points);

  };
}

#endif //LIBOSMSCOUT_TRIANGULATE_H
