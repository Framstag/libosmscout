/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2011  Tim Teulings

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


#include <osmscout/Database.h>
#include <osmscout/StyleConfigLoader.h>
#include <osmscout/Util.h>

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/LookupPOI ../TravelJinni/ 51.2 6.5 51.7 8 amenity_hospital amenity_hospital_building
*/

int main(int argc, char* argv[])
{
  std::string                   map;
  double                        latTop,latBottom,lonLeft,lonRight;
  std::list<std::string>        typeNames;
  std::vector<osmscout::TypeId> types;

  if (argc<6) {
    std::cerr << "LookupPOI <map directory> <lat_top> <lon_left> <lat_bottom> <lon_right> {type}" << std::endl;
    return 1;
  }

  map=argv[1];

  if (sscanf(argv[2],"%lf",&latTop)!=1) {
    std::cerr << "lat_top is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[3],"%lf",&lonLeft)!=1) {
    std::cerr << "lon_left is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[4],"%lf",&latBottom)!=1) {
    std::cerr << "lat_bottom is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[5],"%lf",&lonRight)!=1) {
    std::cerr << "lon_right is not numeric!" << std::endl;
    return 1;
  }

  for (int i=6; i<argc; i++) {
    typeNames.push_back(std::string(argv[i]));
  }


  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  types.reserve(typeNames.size()*3); // Avoid dynamic resize

  for (std::list<std::string>::const_iterator name=typeNames.begin();
      name!=typeNames.end();
      name++) {
    osmscout::TypeId nodeType;
    osmscout::TypeId wayType;
    osmscout::TypeId areaType;

    nodeType=database.GetTypeConfig()->GetNodeTypeId(*name);
    wayType=database.GetTypeConfig()->GetWayTypeId(*name);
    areaType=database.GetTypeConfig()->GetAreaTypeId(*name);

    if (nodeType==osmscout::typeIgnore &&
        wayType==osmscout::typeIgnore &&
        areaType==osmscout::typeIgnore) {
      std::cerr << "Cannot resolve type name '" << *name << "'" << std::endl;
      continue;
    }

    if (nodeType!=osmscout::typeIgnore) {
      types.push_back(nodeType);
    }

    if (wayType!=osmscout::typeIgnore && wayType!=nodeType) {
      types.push_back(wayType);
    }

    if (areaType!=osmscout::typeIgnore && areaType!=nodeType && areaType!=wayType) {
      types.push_back(areaType);
    }

  }

  std::vector<osmscout::NodeRef> nodes;
  std::vector<osmscout::WayRef> ways;
  std::vector<osmscout::WayRef> areas;
  std::vector<osmscout::RelationRef> relationWays;
  std::vector<osmscout::RelationRef> relationAreas;

  osmscout::TagId nameTagId=database.GetTypeConfig()->GetTagId("name");

  if (!database.GetObjects(std::min(lonLeft,lonRight),
                           std::min(latTop,latBottom),
                           std::max(lonLeft,lonRight),
                           std::max(latTop,latBottom),
                           types,
                           nodes,
                           ways,
                           areas,
                           relationWays,
                           relationAreas)) {
    std::cerr << "Cannot load data from database" << std::endl;

    return 1;
  }

  for (std::vector<osmscout::NodeRef>::const_iterator node=nodes.begin();
      node!=nodes.end();
      node++) {
    std::string name;

    if (nameTagId!=osmscout::tagIgnore)
    {
      for (size_t i=0; i<(*node)->GetTagCount(); i++) {
        if ((*node)->GetTagKey(i)==nameTagId) {
          name=(*node)->GetTagValue(i);
          break;
        }
      }
    }

    std::cout << "Node " << (*node)->GetId() << " " << name << std::endl;
  }

  for (std::vector<osmscout::WayRef>::const_iterator way=ways.begin();
      way!=ways.end();
      way++) {
    std::cout << "Way " << (*way)->GetId() << " " << (*way)->GetName() << std::endl;
  }

  for (std::vector<osmscout::RelationRef>::const_iterator way=relationWays.begin();
      way!=relationWays.end();
      way++) {
    std::cout << "Way " << (*way)->GetId() << " " << (*way)->GetName() << std::endl;
  }

  for (std::vector<osmscout::WayRef>::const_iterator area=areas.begin();
      area!=areas.end();
      area++) {
    std::cout << "Area " << (*area)->GetId() << " " << (*area)->GetName() << std::endl;
  }

  for (std::vector<osmscout::RelationRef>::const_iterator area=relationAreas.begin();
      area!=relationAreas.end();
      area++) {
    std::cout << "Area " << (*area)->GetId() << " " << (*area)->GetName() << std::endl;
  }

  return 0;
}
