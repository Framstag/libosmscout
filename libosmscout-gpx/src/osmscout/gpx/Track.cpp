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

#include <osmscout/gpx/Track.h>

using namespace osmscout;
using namespace osmscout::gpx;

size_t Track::GetPointCount() const
{
  size_t result=0;
  for (const auto &segment:segments){
    result+=segment.points.size();
  }
  return result;
}

double Track::GetLength() const
{
  double result=0;
  for (const auto &segment:segments){
    result+=segment.GetLength();
  }
  return result;
}

void Track::FilterPoints(std::function<void(std::vector<TrackPoint> &)> filter)
{
  for (auto &segment:segments){
    filter(segment.points);
  }
}

