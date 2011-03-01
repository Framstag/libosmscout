/*
  Routing - a demo program for libosmscout
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

#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>

/*
  Example for the nordrhein-westfalen.osm:
  time src/Routing ../TravelJinni/ 14332719 138190834 10414977 283372120
*/

int main(int argc, char* argv[])
{
  std::string   map;
  unsigned long startWayId;
  unsigned long startNodeId;
  unsigned long targetWayId;
  unsigned long targetNodeId;

  if (argc!=6) {
    std::cerr << "Routing <map directory> <start way id> <start node id> <target way id> <target node id>" << std::endl;
    return 1;
  }

  map=argv[1];

  if (sscanf(argv[2],"%lu",&startWayId)!=1) {
    std::cerr << "start way id is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[3],"%lu",&startNodeId)!=1) {
    std::cerr << "start node id is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[4],"%lu",&targetWayId)!=1) {
    std::cerr << "target way id is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lu",&targetNodeId)!=1) {
    std::cerr << "target node id is not numeric!" << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::RouteData        data;
  osmscout::RouteDescription description;

  if (!database.CalculateRoute(startWayId,startNodeId,
                               targetWayId,targetNodeId,
                               data)) {
    std::cerr << "There was an error while calculating the route!" << std::endl;
    database.Close();
    return 1;
  }

  database.TransformRouteDataToRouteDescription(data,description);

  double lastDistance = 0;

  for (std::list<osmscout::RouteDescription::RouteStep>::const_iterator step=description.Steps().begin();
       step!=description.Steps().end();
       ++step) {
#if defined(HTML)
    std::cout << "<tr><td>";
#endif
    std::cout << std::fixed << std::setprecision(1);
    std::cout << step->GetAt() << "km ";

    if (step->GetAfter()!=0.0) {
      std::cout << std::fixed << std::setprecision(1);
      std::cout << step->GetAfter() << "km ";
    }
    else {
      std::cout << "      ";
    }

#if defined(HTML)
    std::cout <<"</td>";
#endif

#if defined(HTML)
    std::cout << "<td>";
#endif
    switch (step->GetAction()) {
    case osmscout::RouteDescription::start:
      std::cout << "Start at ";
      if (!step->GetName().empty()) {
        std::cout << step->GetName();

        if (!step->GetRefName().empty()) {
          std::cout << " (" << step->GetRefName() << ")";
        }
      }
      else {
        std::cout << step->GetRefName();
      }
      break;
    case osmscout::RouteDescription::drive:
      std::cout << "drive along ";
      if (!step->GetName().empty()) {
        std::cout << step->GetName();

        if (!step->GetRefName().empty()) {
          std::cout << " (" << step->GetRefName() << ")";
        }
      }
      else {
        std::cout << step->GetRefName();
      }
      break;
    case osmscout::RouteDescription::switchRoad:
      std::cout << "turn into ";
      if (!step->GetName().empty()) {
        std::cout << step->GetName();

        if (!step->GetRefName().empty()) {
          std::cout << " (" << step->GetRefName() << ")";
        }
      }
      else {
        std::cout << step->GetRefName();
      }
      break;
    case osmscout::RouteDescription::reachTarget:
      std::cout << "Arriving at ";
      if (!step->GetName().empty()) {
        std::cout << step->GetName();

        if (!step->GetRefName().empty()) {
          std::cout << " (" << step->GetRefName() << ")";
        }
      }
      else {
        std::cout << step->GetRefName();
      }
      break;
    case osmscout::RouteDescription::pass:
      std::cout << "passing along ";
      if (!step->GetName().empty()) {
        std::cout << step->GetName();

        if (!step->GetRefName().empty()) {
          std::cout << " (" << step->GetRefName() << ")";
        }
      }
      else {
        std::cout << step->GetRefName();
      }
      break;
    }

#if defined(HTML)
    std::cout << "</td></tr>";
#endif
    std::cout << std::endl;

    lastDistance=step->GetAt();
  }


  database.Close();

  return 0;
}