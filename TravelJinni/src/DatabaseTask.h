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

#include <Lum/Features.h>

#if defined(LUM_HAVE_LIB_CAIRO)
  #include <Lum/OS/Cairo/Bitmap.h>
  #include <Lum/OS/Cairo/DrawInfo.h>
#endif

#include <Lum/OS/Bitmap.h>
#include <Lum/OS/Thread.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/MapPainterCairo.h>

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
  osmscout::Database        *database;
  osmscout::StyleConfig     *styleConfig;
  Lum::OS::Condition        condition;
  mutable Lum::OS::Mutex    mutex;
  bool                      finish;
  Job                       *newJob;
  Job                       *currentJob;
  Job                       *finishedJob;
  Lum::Model::Action        *jobFinishedAction;
  cairo_surface_t           *currentSurface;
  cairo_t                   *currentCairo;
  size_t                    currentWidth,currentHeight;
  double                    currentLon,currentLat;
  double                    currentMagnification;
  cairo_surface_t           *finishedSurface;
  cairo_t                   *finishedCairo;
  size_t                    finishedWidth,finishedHeight;
  double                    finishedLon,finishedLat;
  double                    finishedMagnification;
  osmscout::MapPainterCairo painter;

private:
  void SignalRedraw();

public:
  DatabaseTask(osmscout::Database* database,
               Lum::Model::Action* jobFinishedAction);

  void Run();
  void Finish();

  bool Open(const std::wstring& path);
  bool IsOpen() const;
  void Close();

  void FlushCache();

  bool LoadStyleConfig(const std::wstring& filename,
                       osmscout::StyleConfig*& styleConfig);
  void SetStyle(osmscout::StyleConfig* styleConfig);


  bool GetBoundingBox(double& minLat,double& minLon,
                      double& maxLat,double& maxLon) const;

  bool GetWay(osmscout::Id id, osmscout::Way& way) const;

  bool GetMatchingAdminRegions(const std::wstring& name,
                               std::list<osmscout::AdminRegion>& regions,
                               size_t limit,
                               bool& limitReached) const;

  bool GetMatchingLocations(const osmscout::AdminRegion& region,
                            const std::wstring& name,
                            std::list<osmscout::Location>& locations,
                            size_t limit,
                            bool& limitReached) const;

  bool CalculateRoute(osmscout::Id startWayId, osmscout::Id startNodeId,
                      osmscout::Id targetWayId, osmscout::Id targetNodeId,
                      osmscout::RouteData& route);
  bool TransformRouteDataToRouteDescription(const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description);
  bool TransformRouteDataToWay(const osmscout::RouteData& data,
                               osmscout::Way& way);
  void ClearRoute();
  void AddRoute(const osmscout::Way& way);

  void PostJob(Job *job);

  bool DrawResult(Lum::OS::Window* window,
                  Lum::OS::DrawInfo* draw,
                  int x, int y,
                  size_t width, size_t height,
                  double lon, double lat,
                  double magnification);
};

#endif
