#ifndef OSMSCOUT_LOCATION_H
#define OSMSCOUT_LOCATION_H

/*
  This source is part of the libosmscout library
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

#include <osmscout/ObjectRef.h>

namespace osmscout {
  /**
    A location is a named point, way, area or relation on the map.
    Something you can search for. Location are currently returned
    by Database.GetMatchingLocations() which uses CityStreetIndex
    internally.
   */
  class OSMSCOUT_API Location
  {
  public:
    std::string            name;
    std::list<std::string> path;
    std::list<ObjectRef>   references;

  public:
    Location();

    bool IsValid() const;
    void Invalidate();
  };
}

#endif
