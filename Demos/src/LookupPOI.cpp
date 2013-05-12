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

/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/LookupPOI ../TravelJinni/ 51.2 6.5 51.7 8 amenity_hospital amenity_hospital_building
  src/LookupPOI ../TravelJinni/ 51.2 6.5 51.7 8 shop
*/

int main(int argc, char* argv[])
{
  std::string            map;
  double                 latTop,latBottom,lonLeft,lonRight;
  std::list<std::string> typeNames;

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

  std::cout << "- Search area: ";
  std::cout << "[" << std::min(latTop,latBottom) << "," << std::min(lonLeft,lonRight) << "]";
  std::cout << "x";
  std::cout << "[" <<std::max(latTop,latBottom) << "," << std::max(lonLeft,lonRight) << "]" << std::endl;

  osmscout::TypeSet types(*database.GetTypeConfig());

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

    std::cout << "- Searching for '" << *name << "' as";

    if (nodeType!=osmscout::typeIgnore) {
      std::cout << " node (" << nodeType << ")";

      types.SetType(nodeType);
    }

    if (wayType!=osmscout::typeIgnore) {
      std::cout << " way (" << wayType << ")";

      types.SetType(wayType);
    }

    if (areaType!=osmscout::typeIgnore) {
      std::cout << " area (" << areaType << ")";

      types.SetType(areaType);
    }

    std::cout << std::endl;
  }

  std::vector<osmscout::NodeRef> nodes;
  std::vector<osmscout::WayRef>  ways;
  std::vector<osmscout::AreaRef> areas;

  osmscout::TagId nameTagId=database.GetTypeConfig()->GetTagId("name");

  if (!database.GetObjects(std::min(lonLeft,lonRight),
                           std::min(latTop,latBottom),
                           std::max(lonLeft,lonRight),
                           std::max(latTop,latBottom),
                           types,
                           nodes,
                           ways,
                           areas)) {
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

    std::cout << "+ Node " << (*node)->GetFileOffset();
    std::cout << " " << database.GetTypeConfig()->GetTypeInfo((*node)->GetType()).GetName();
    std::cout << " " << name << std::endl;
  }

  for (std::vector<osmscout::WayRef>::const_iterator way=ways.begin();
      way!=ways.end();
      way++) {
    std::cout << "+ Way " << (*way)->GetFileOffset();
    std::cout << " " << database.GetTypeConfig()->GetTypeInfo((*way)->GetType()).GetName();
    std::cout << " " << (*way)->GetName() << std::endl;
  }

  for (std::vector<osmscout::AreaRef>::const_iterator area=areas.begin();
      area!=areas.end();
      area++) {
    std::cout << "+ Area " << (*area)->GetFileOffset();
    std::cout << " " << database.GetTypeConfig()->GetTypeInfo((*area)->GetType()).GetName();
    std::cout << " " << (*area)->GetName() << std::endl;
  }

  return 0;
}
