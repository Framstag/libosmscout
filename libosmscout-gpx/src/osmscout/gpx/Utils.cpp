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

void ProcessCallback::Error(std::string error)
{
  osmscout::log.Error() << error;
}


void gpx::FilterNearPoints(std::vector<TrackPoint> &points,
                           double minDistance)
{
  if (points.empty()){
    return;
  }
  std::vector<TrackPoint>::iterator it=points.begin();
  GeoCoord latest=(*it).coord;
  it++;
  while (it!=points.end()){
    GeoCoord current=(*it).coord;
    double distance=GetEllipsoidalDistance(latest, current)*1000;
    if (distance <= minDistance){
      it = points.erase(it);
    }else {
      latest=current;
      it++;
    }
  }
}

void gpx::FilterInaccuratePoints(std::vector<TrackPoint> &points,
                                 double maxDilution)
{
  std::remove_if(points.begin(),
                 points.end(),
                 [&maxDilution](const TrackPoint &p) {
                   if (p.hdop.hasValue()){
                     return p.hdop.get() > maxDilution;
                   }
                   if (p.pdop.hasValue()){
                     return p.pdop.get() > maxDilution;
                   }
                   return false;
                 });
}
