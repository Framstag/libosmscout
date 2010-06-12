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

#include <osmscout/MapPainter.h>

namespace osmscout {

  static const double gradtorad=2*M_PI/360;

  MapPainter::MapPainter(const Database& database)
   : database(database)
  {
    projection=new MercatorProjection();
  }

  MapPainter::~MapPainter()
  {
    // no code
  }
  
  void MapPainter::SetProjection(Projection* projection)
  {
    if (projection==NULL) {
      return;
    }
    
    delete this->projection;
    this->projection=projection;
  }  
}

