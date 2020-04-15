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

#include <osmscout/gpx/Utils.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

#include <algorithm>

using namespace osmscout;
using namespace osmscout::gpx;

void ProcessCallback::Progress(double)
{
  // no-op
}

void ProcessCallback::Error(const std::string& error)
{
  osmscout::log.Error() << error;
}


void gpx::FilterNearPoints(std::vector<TrackPoint> &points,
                           const Distance &minDistance)
{
  if (points.empty()){
    return;
  }
  GeoCoord latest=points[0].coord;
  std::vector<TrackPoint> copy;
  copy.reserve(points.size());
  bool modified = false;
  for (const TrackPoint &current:points){
    Distance distance=GetEllipsoidalDistance(latest, current.coord);
    if (distance > minDistance){
      latest = current.coord;
      copy.push_back(current);
    }else{
      modified = true;
    }
  }
  if (modified) {
    points = copy;
  }
}

void gpx::FilterInaccuratePoints(std::vector<TrackPoint> &points,
                                 double maxDilution)
{
  auto filter=[&maxDilution](const TrackPoint &p) {
    if (p.hdop){
      return *p.hdop > maxDilution;
    }
    if (p.pdop){
      return *p.pdop > maxDilution;
    }
    return false;
  };

  points.erase(std::remove_if(points.begin(),points.end(),filter),points.end());
}
