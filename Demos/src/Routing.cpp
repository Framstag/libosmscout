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

#include <osmscout/Database.h>

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

  Database database;

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  RouteData data;

  if (!database.CalculateRoute(startWayId,startNodeId,
                               targetWayId,targetNodeId,
                               data)) {
    std::cerr << "There was an error while calculating the route!" << std::endl;
    database.Close();
    return 1;
  }

  database.Close();

  return 0;
}