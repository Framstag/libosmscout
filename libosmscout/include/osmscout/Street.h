#ifndef OSMSCOUT_STREET_H
#define OSMSCOUT_STREET_H

/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Reference.h>

/**
  A street represented (normally) eitehr by a way or an area.

  Street is in fact a bad name, since a place in form of an area can also
  be a possible result.
 */
struct Street
{
  Reference   reference;
  std::string name;
};

#endif
