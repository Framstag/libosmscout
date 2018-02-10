/*
  OSMScout2 - demo application for libosmscout
  Copyright (C) 2017 Lukas Karas

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

#include "PositionSimulator.h"

#include <osmscout/GPXFeatures.h>
#include <osmscout/gpx/Import.h>
#include <osmscout/util/Logger.h>

static const std::chrono::milliseconds TickDuration(100);

PositionSimulator::PositionSimulator() {
  connect(&timer, SIGNAL(timeout()),
          this, SLOT(tick()));
  timer.setInterval(TickDuration.count());
  timer.setSingleShot(false);
}

void PositionSimulator::setTrack(const QString &t)
{
  trackFile=t;
  setRunning(false);
  fileLoaded=false;

  osmscout::gpx::GpxFile gpxFile;

#ifndef OSMSCOUT_GPX_HAVE_LIB_XML
  osmscout::log.Error() << "GPX import is not supported!";
  return;
#else
  osmscout::log.Info() << "Loading " << trackFile.toStdString();
  if (!osmscout::gpx::ImportGpx(trackFile.toStdString(), gpxFile)){
    osmscout::log.Error() << "Failed to load gpx file " << trackFile.toStdString();
    return;
  }
#endif

  segments.clear();

  for (auto const &trk:gpxFile.tracks){
    for (auto const &seg:trk.segments){
      if (seg.points.empty()){
        continue;
      }
      segments.push_back(seg);
    }
  }
  if (segments.empty()){
    osmscout::log.Error() << "No track in gpx file " << trackFile.toStdString();
    return;
  }

  fileLoaded=true;
  if (!setSegment(0)){
    return;
  }
  setRunning(true);
}

osmscout::Timestamp Now(){
  using namespace std::chrono;
  return time_point_cast<milliseconds,system_clock,nanoseconds>(osmscout::Timestamp::clock::now());
}

QDateTime PositionSimulator::getTime() const
{
  return QDateTime::fromMSecsSinceEpoch(simulationTime.time_since_epoch().count());
}

void PositionSimulator::skipTime(uint64_t millis)
{
  simulationTime+=std::chrono::milliseconds(millis);
  emit timeChanged(getTime());
}

bool PositionSimulator::setSegment(size_t i)
{
  currentSegment=i;
  currentPoint=0;
  if (currentSegment>=segments.size()){
    return false;
  }
  auto &points=segments[currentSegment].points;
  if (points.empty()){
    return false; // should not happen
  }
  segmentStart=points[0];
  segmentEnd=points[points.size()-1];

  simulationTime=segmentStart.time.getOrElse([]()->osmscout::Timestamp{
    return Now();
  });
  emit startChanged(segmentStart.coord.GetLat(), segmentStart.coord.GetLon());
  emit endChanged(segmentEnd.coord.GetLat(), segmentEnd.coord.GetLon());

  emit timeChanged(getTime());
  return true;
}

void PositionSimulator::setRunning(bool b)
{
  if (running==b){
    return;
  }
  if (b && !fileLoaded){
    return;
  }
  running=b;
  emit runningChanged(running);
  if (running) {
    osmscout::log.Debug() << "Simulator started";
    timer.start();
  }else{
    osmscout::log.Debug() << "Simulator stopped";
    timer.stop();
  }
}

void PositionSimulator::tick()
{
  if (currentSegment>=segments.size()){
    setRunning(false);
    return;
  }
  auto &points=segments[currentSegment].points;
  if (points.empty()){
    return; // should not happen
  }
  simulationTime+=TickDuration;
  // osmscout::log.Debug() << "Simulator time: " << osmscout::TimestampToISO8601TimeString(simulationTime);
  emit timeChanged(getTime());

  while (true){
    if (currentPoint<points.size()) {
      auto &point=points[currentPoint];
      if (point.time.hasValue() && point.time.get()>simulationTime){
        return;
      }
      osmscout::log.Debug() << "Simulator point: " << osmscout::TimestampToISO8601TimeString(point.time.getOrElse(simulationTime)) << " @ " << point.coord.GetDisplayText();
      currentPosition=point.coord;
      emit positionChanged(currentPosition.GetLat(), currentPosition.GetLon(), point.hdop.hasValue(), point.hdop.getOrElse(0));
      currentPoint++;
      if (!point.time.hasValue()){
        return;
      }
    }else{
      setSegment(currentSegment+1);
      tick();
      return;
    }
  }
}
