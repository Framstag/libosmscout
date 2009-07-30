#ifndef DATABASETASK_H
#define DATABASETASK_H

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

#include <Lum/OS/Cairo/Bitmap.h>
#include <Lum/OS/Cairo/DrawInfo.h>

#include <Lum/OS/Bitmap.h>
#include <Lum/OS/Thread.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

#include "MapPainter.h"

struct Job
{
  double          lon;
  double          lat;
  double          magnification;
  size_t          width;
  size_t          height;
};

class DatabaseTask : public Lum::OS::Thread
{
private:
  Database           *database;
  StyleConfig        *styleConfig;
  Lum::OS::Condition condition;
  mutable Lum::OS::Mutex mutex;
  bool               finish;
  Job                *newJob;
  Job                *currentJob;
  Job                *finishedJob;
  Lum::Model::Action *jobFinishedAction;
  cairo_surface_t    *currentSurface;
  cairo_t            *currentCairo;
  size_t             currentWidth,currentHeight;
  double             currentLon,currentLat;
  double             currentMagnification;
  cairo_surface_t    *finishedSurface;
  cairo_t            *finishedCairo;
  size_t             finishedWidth,finishedHeight;
  double             finishedLon,finishedLat;
  double             finishedMagnification;
  MapPainter         painter;

private:
  void SignalRedraw();

public:
  DatabaseTask(Database* database,
               Lum::Model::Action* jobFinishedAction);

  void Run();
  void Finish();

  bool Open(const std::wstring& path);
  bool IsOpen() const;
  void Close();

  bool LoadStyleConfig(const std::wstring& filename, StyleConfig*& styleConfig);
  void SetStyle(StyleConfig* styleConfig);

  bool GetWay(Id id, Way& way) const;

  bool GetMatchingCities(const std::wstring& name,
                         std::list<City>& cities,
                         size_t limit,
                         bool& limitReached) const;

  bool GetMatchingStreets(Id urbanId, const std::wstring& name,
                          std::list<Street>& streets,
                          size_t limit, bool& limitReached) const;

  bool CalculateRoute(Id startWayId, Id startNodeId,
                      Id targetWayId, Id targetNodeId,
                      RouteData& route);
  bool TransformRouteDataToRouteDescription(const RouteData& data,
                                            RouteDescription& description);
  bool TransformRouteDataToWay(const RouteData& data,
                               Way& way);
  void ClearRoute();
  void AddRoute(const Way& way);

  void PostJob(Job *job);

  bool DrawResult(Lum::OS::DrawInfo* draw,
                  int x, int y,
                  size_t width, size_t height,
                  double lon, double lat,
                  double magnification);
};

#endif
