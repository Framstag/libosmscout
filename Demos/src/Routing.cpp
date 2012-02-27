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
#include <list>

#include <osmscout/Database.h>
#include <osmscout/Router.h>

/*
  Examples for the nordrhein-westfalen.osm:

  Long:
  time src/Routing ../TravelJinni/ 14332719 138190834 10414977 283372120

  Short:
  time src/Routing ../TravelJinni/ 33879912 388178746 24922615 270813911
*/

static bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
{
  return node.HasDescription(osmscout::RouteDescription::NODE_START_DESC) ||
         node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC) ||
         node.HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
}

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

  osmscout::RouterParameter routerParameter;
  osmscout::Router          router(routerParameter);

  if (!router.Open(map.c_str())) {
    std::cerr << "Cannot open routing database" << std::endl;

    return 1;
  }

  osmscout::TypeId           type;
  osmscout::TypeConfig       *typeConfig=router.GetTypeConfig();

  osmscout::RoutingProfile   routingProfile;
  osmscout::RouteData        data;
  osmscout::RouteDescription description;

  routingProfile.SetTurnCostFactor(1/60/2); // 30 seconds

  type=typeConfig->GetWayTypeId("highway_motorway");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/110.0);

  type=typeConfig->GetWayTypeId("highway_motorway_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/60.0);

  type=typeConfig->GetWayTypeId("highway_trunk");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/70.0);

  type=typeConfig->GetWayTypeId("highway_trunk_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/70.0);

  type=typeConfig->GetWayTypeId("highway_primary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/70.0);

  type=typeConfig->GetWayTypeId("highway_primary_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/60.0);

  type=typeConfig->GetWayTypeId("highway_secondary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/60.0);

  type=typeConfig->GetWayTypeId("highway_secondary_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId("highway_tertiary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/55.0);

  type=typeConfig->GetWayTypeId("highway_unclassified");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId("highway_road");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId("highway_residential");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/40.0);

  type=typeConfig->GetWayTypeId("highway_living_street");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/10.0);

  type=typeConfig->GetWayTypeId("highway_service");
  assert(type!=osmscout::typeIgnore);
  routingProfile.SetTypeCostFactor(type,1/30.0);

  if (!router.CalculateRoute(routingProfile,
                             startWayId,startNodeId,
                             targetWayId,targetNodeId,
                             data)) {
    std::cerr << "There was an error while calculating the route!" << std::endl;
    router.Close();
    return 1;
  }

  router.TransformRouteDataToRouteDescription(data,description);

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(new osmscout::RoutePostprocessor::DistancePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::StartPostprocessor("Start"));
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNamePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNameChangedPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::TargetPostprocessor("Target"));

  osmscout::DatabaseParameter  databaseParameter;
  osmscout::Database           database(databaseParameter);
  osmscout::RoutePostprocessor postprocessor;

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  if (!postprocessor.PostprocessRouteDescription(description,
                                                 database,
                                                 postprocessors)) {
    std::cerr << "Error during route postprocessing" << std::endl;
  }

  std::cout << "-----" << std::endl;
  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=description.Nodes().end();
  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=description.Nodes().begin();
       node!=description.Nodes().end();
       ++node) {
    if (!HasRelevantDescriptions(*node)) {
      continue;
    }

#if defined(HTML)
    std::cout << "<tr><td>";
#endif
    std::cout << std::setfill(' ') << std::setw(5) << std::fixed << std::setprecision(1);
    std::cout << node->GetDistance() << "km ";

    if (prevNode!=description.Nodes().end() && node->GetDistance()-prevNode->GetDistance()!=0.0) {
      std::cout << std::setfill(' ') << std::setw(4) << std::fixed << std::setprecision(1);
      std::cout << node->GetDistance()-prevNode->GetDistance() << "km ";
    }
    else {
      std::cout << "       ";
    }

#if defined(HTML)
    std::cout <<"</td>";
#endif

#if defined(HTML)
    std::cout << "<td>";
#endif

    size_t lineCount=0;

    if (node->HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
      osmscout::RouteDescription::DescriptionRef   description=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
      osmscout::RouteDescription::StartDescription *startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(description.Get());

      if (lineCount>0) {
        std::cout << "               ";
      }

      std::cout << "Start at \"" << startDescription->GetDescription() << "\"" << std::endl;
      lineCount++;
    }

    if (node->HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
      osmscout::RouteDescription::DescriptionRef    description=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
      osmscout::RouteDescription::TargetDescription *targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(description.Get());

      if (lineCount>0) {
        std::cout << "               ";
      }

      std::cout << "Target reached \"" << targetDescription->GetDescription() << "\"" << std::endl;
      lineCount++;
    }

    if (node->HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC)) {
      osmscout::RouteDescription::DescriptionRef  description=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
      osmscout::RouteDescription::NameDescription *nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(description.Get());

      if (lineCount>0) {
        std::cout << "               ";
      }

      std::cout << "Way ";
      if (!nameDescription->GetName().empty()) {
        std::cout << nameDescription->GetName();
      }

      if (!nameDescription->GetName().empty() &&
          !nameDescription->GetRef().empty()) {
        std::cout << " (";
      }

      if (!nameDescription->GetRef().empty()) {
        std::cout << nameDescription->GetRef();
      }

      if (!nameDescription->GetName().empty() &&
          !nameDescription->GetRef().empty()) {
        std::cout << ")";
      }

      std::cout << std::endl;

      lineCount++;
    }

#if defined(HTML)
    std::cout << "</td></tr>";
#endif

    prevNode=node;
  }
  std::cout << "-----" << std::endl;

  router.Close();

  return 0;
}
