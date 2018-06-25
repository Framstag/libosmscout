/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/TrackSegment.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Distance.h>

using namespace osmscout;
using namespace osmscout::gpx;

Distance TrackSegment::GetLength() const
{
  Distance result;
  auto it=points.begin();
  if (it==points.end()){
    return result;
  }
  GeoCoord previous=it->coord;
  it++;
  for (; it!=points.end(); it++){
    result+=GetEllipsoidalDistance(previous, it->coord);
    previous=it->coord;
  }
  return result;
}