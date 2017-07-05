/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/routing/RoutingService.h>

namespace osmscout {

  RoutePosition::RoutePosition()
  : nodeIndex(0)
  {
    // no code
  }

  RoutePosition::RoutePosition(const ObjectFileRef& object,
                               size_t nodeIndex,
                               DatabaseId database)
  : object(object),
    nodeIndex(nodeIndex),
    database(database)
  {
    // no code
  }

  RouterParameter::RouterParameter()
  : debugPerformance(false)
  {
    // no code
  }

  void RouterParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  bool RouterParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  RoutingProgress::~RoutingProgress()
  {
    // no code
  }

  void RoutingParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }

  void RoutingParameter::SetProgress(const RoutingProgressRef& progress)
  {
    this->progress=progress;
  }

  RoutingResult::RoutingResult()
  : currentMaxDistance(0.0),
    overallDistance(0.0)
  {
    // no code
  }

  std::string RoutingService::GetDataFilename(const std::string& filenamebase)
  {
    return filenamebase+".dat";
  }

  std::string RoutingService::GetData2Filename(const std::string& filenamebase)
  {
    return filenamebase+"2.dat";
  }

  std::string RoutingService::GetIndexFilename(const std::string& filenamebase)
  {
    return filenamebase+".idx";
  }

  const char* const RoutingService::FILENAME_INTERSECTIONS_DAT   = "intersections.dat";
  const char* const RoutingService::FILENAME_INTERSECTIONS_IDX   = "intersections.idx";

  const char* const RoutingService::DEFAULT_FILENAME_BASE        = "router";

  RoutingService::RoutingService()
  {
  }

  RoutingService::~RoutingService()
  {
  }
}
