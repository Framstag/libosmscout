#ifndef OSMSCOUT_MAP_MAPPAINTER_H
#define OSMSCOUT_MAP_MAPPAINTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <set>

#include <osmscout/Database.h>
#include <osmscout/Projection.h>

#include <osmscout/Private/MapImportExport.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapPainter
  {
  protected:
    const Database& database;

    Projection      *projection;   //! Projection to use

  public:
    std::list<Way>  poiWays;
    std::list<Node> poiNodes;

  protected:
    void RecalculateData(double lon, double lat,
                         double magnification,
                         size_t width, size_t height);

  public:
    MapPainter(const Database& database);
    virtual ~MapPainter();

    void SetProjection(Projection* projection);
  };
}

#endif
