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

#include <cmath>
#include <iostream>
#include <iomanip>
#include <list>
#include <sstream>

#include <osmscout/Database.h>
#include <osmscout/Router.h>

//#define ROUTE_DEBUG
//#define NODE_DEBUG
//#define CROSSING_DEBUG

/*
  Examples for the nordrhein-westfalen.osm:

  Long:
  time src/Routing ../TravelJinni/ 14332719 138190834 10414977 283372120

  Medium:
  time src/Routing ../TravelJinni/ 33879912 388178746 38363871 453298626

  Short:
  time src/Routing ../TravelJinni/ 33879912 388178746 24922615 270813911
*/

static std::string TimeToString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

  return stream.str();
}

static bool IsRelevantCrossing(const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossing)
{
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossing->GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossing->GetTargetDesccription();

  if (originDescription.Valid() &&
      targetDescription.Valid()) {
    // Is not really a turn, because there are no other crossing ways
    if (crossing->GetDescriptions().empty()) {
      return false;
    }

    // It is a crossing with additional ways, but we stay on the same way
    if (originDescription->GetName()==targetDescription->GetName() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return false;
    }

    // Origin and target way just change refs
    if (!originDescription->GetName().empty() &&
        originDescription->GetName()==targetDescription->GetName()) {
      return false;
    }

    // The name changes from empty to non-empty, but the ref stays the same
    if (originDescription->GetName().empty() &&
        !targetDescription->GetName().empty() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return false;
    }

    // The name changes from non-empty to empty, but the ref stays the same
    if (!originDescription->GetName().empty() &&
        targetDescription->GetName().empty() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return false;
    }

    // Target way does not have a name at all :-/
    if (targetDescription->GetName().empty() &&
        targetDescription->GetRef().empty()) {
      return false;
    }
  }

  return true;
}

static bool IsRelevantNameChange(const osmscout::RouteDescription::NameChangedDescriptionRef& change)
{
  osmscout::RouteDescription::NameDescriptionRef originDescription=change->GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=change->GetTargetDesccription();

  if (originDescription.Valid() &&
      targetDescription.Valid()) {
    // The name changes from empty to non-empty, but the ref stays the same
    if (originDescription->GetName().empty() &&
        !targetDescription->GetName().empty() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return false;
    }

    // The name changes from non-empty to empty, but the ref stays the same
    if (!originDescription->GetName().empty() &&
        targetDescription->GetName().empty() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return false;
    }

    // ref changes, but name stays the same
    if (originDescription->GetName()==targetDescription->GetName()) {
      return false;
    }
  }

  return true;
}

static bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
{
#if defined(ROUTE_DEBUG)
  return true;
#else
  if (node.HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC)) {
    osmscout::RouteDescription::DescriptionRef             description=node.GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    osmscout::RouteDescription::CrossingWaysDescriptionRef crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(description.Get());

    if (IsRelevantCrossing(crossingWaysDescription)) {
      return true;
    }
  }

  if (node.HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC)) {
    osmscout::RouteDescription::DescriptionRef            description=node.GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    osmscout::RouteDescription::NameChangedDescriptionRef nameChangedDescription=dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>(description.Get());

    if (IsRelevantNameChange(nameChangedDescription)) {
      return true;
    }
  }

  return false;
#endif
}

static std::string NameDescriptionToString(osmscout::RouteDescription::NameDescription* nameDescription)
{
  std::ostringstream stream;

  if (!nameDescription->GetName().empty()) {
    stream << nameDescription->GetName();
  }

  if (!nameDescription->GetName().empty() &&
      !nameDescription->GetRef().empty()) {
    stream << " (";
  }

  if (!nameDescription->GetRef().empty()) {
    stream << nameDescription->GetRef();
  }

  if (!nameDescription->GetName().empty() &&
      !nameDescription->GetRef().empty()) {
    stream << ")";
  }

  return stream.str();
}

bool IsJustWayChange(osmscout::RouteDescription::CrossingWaysDescription* crossingDescription)
{
  if (crossingDescription==NULL) {
    return true;
  }

  if (crossingDescription->GetDescriptions().size()==0) {
    return true;
  }

  osmscout::RouteDescription::NameDescriptionRef originDescription=crossingDescription->GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingDescription->GetTargetDesccription();

  if (originDescription.Valid() &&
      targetDescription.Valid()) {
    // Origin and target way just change refs
    if (!originDescription->GetName().empty() &&
        originDescription->GetName()==targetDescription->GetName()) {
      return true;
    }

    // Origin and target way just change names
    if (!originDescription->GetRef().empty() &&
        originDescription->GetRef()==targetDescription->GetRef()) {
      return true;
    }
  }

  return false;
}

static void NextLine(size_t& lineCount)
{
  if (lineCount>0) {
    std::cout << "                             ";
  }

  lineCount++;
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

  osmscout::TypeId                    type;
  osmscout::TypeConfig                *typeConfig=router.GetTypeConfig();

  //osmscout::ShortestPathRoutingProfile routingProfile;
  osmscout::FastestPathRoutingProfile routingProfile;
  osmscout::RouteData                 data;
  osmscout::RouteDescription          description;

  routingProfile.SetVehicleMaxSpeed(160.0);

  type=typeConfig->GetWayTypeId("highway_motorway");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,110.0);

  type=typeConfig->GetWayTypeId("highway_motorway_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,60.0);

  type=typeConfig->GetWayTypeId("highway_trunk");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,70.0);

  type=typeConfig->GetWayTypeId("highway_trunk_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,70.0);

  type=typeConfig->GetWayTypeId("highway_primary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,70.0);

  type=typeConfig->GetWayTypeId("highway_primary_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,60.0);

  type=typeConfig->GetWayTypeId("highway_secondary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,60.0);

  type=typeConfig->GetWayTypeId("highway_secondary_link");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,50.0);

  type=typeConfig->GetWayTypeId("highway_tertiary");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,55.0);

  type=typeConfig->GetWayTypeId("highway_unclassified");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,50.0);

  type=typeConfig->GetWayTypeId("highway_road");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,50.0);

  type=typeConfig->GetWayTypeId("highway_residential");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,40.0);

  type=typeConfig->GetWayTypeId("highway_living_street");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,10.0);

  type=typeConfig->GetWayTypeId("highway_service");
  assert(type!=osmscout::typeIgnore);
  routingProfile.AddType(type,30.0);

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

  postprocessors.push_back(new osmscout::RoutePostprocessor::DistanceAndTimePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::StartPostprocessor("Start"));
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNamePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNameChangedPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::CrossingWaysPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::TargetPostprocessor("Target"));

  osmscout::DatabaseParameter  databaseParameter;
  osmscout::Database           database(databaseParameter);
  osmscout::RoutePostprocessor postprocessor;

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  if (!postprocessor.PostprocessRouteDescription(description,
                                                 routingProfile,
                                                 database,
                                                 postprocessors)) {
    std::cerr << "Error during route postprocessing" << std::endl;
  }

  std::cout << "----------------------------------------------------" << std::endl;
  std::cout << "     At| After|  Time| After|" << std::endl;
  std::cout << "----------------------------------------------------" << std::endl;
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

#if defined(HTML)
    std::cout <<"</td><td>";
#endif

    if (prevNode!=description.Nodes().end() && node->GetDistance()-prevNode->GetDistance()!=0.0) {
      std::cout << std::setfill(' ') << std::setw(4) << std::fixed << std::setprecision(1);
      std::cout << node->GetDistance()-prevNode->GetDistance() << "km ";
    }
    else {
      std::cout << "       ";
    }

#if defined(HTML)
    std::cout << "<tr><td>";
#endif
    std::cout << TimeToString(node->GetTime()) << "h ";

#if defined(HTML)
    std::cout <<"</td><td>";
#endif

    if (prevNode!=description.Nodes().end() && node->GetTime()-prevNode->GetTime()!=0.0) {
      std::cout << TimeToString(node->GetTime()-prevNode->GetTime()) << "h ";
    }
    else {
      std::cout << "       ";
    }


#if defined(HTML)
    std::cout <<"</td><td>";
#endif

    size_t lineCount=0;

    osmscout::RouteDescription::DescriptionRef             description;
    osmscout::RouteDescription::StartDescriptionRef        startDescription;
    osmscout::RouteDescription::TargetDescriptionRef       targetDescription;
    osmscout::RouteDescription::NameChangedDescriptionRef  nameChangedDescription;
    osmscout::RouteDescription::CrossingWaysDescriptionRef crossingWaysDescription;

    description=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
    if (description.Valid()) {
      startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(description.Get());
    }

    description=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
    if (description.Valid()) {
      targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(description.Get());
    }

    description=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    if (description.Valid()) {
      nameChangedDescription=dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>(description.Get());
    }

    description=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    if (description.Valid()) {
      crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(description.Get());
    }

#if defined(ROUTE_DEBUG) || defined(NODE_DEBUG)
    NextLine(lineCount);

    std::cout << node->GetCurrentNodeId() << " => " << node->GetPathWayId() << "[" << node->GetTargetNodeId() << "]" << std::endl;
#endif

#if defined(ROUTE_DEBUG) || defined(CROSSING_DEBUG)
    if (crossingWaysDescription.Valid()) {
      osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription->GetOriginDesccription();
      osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription->GetTargetDesccription();

      NextLine(lineCount);
      std::cout << "Crossing";

      if (originDescription.Valid()) {
        std::cout << " from " << NameDescriptionToString(originDescription);
      }

      if (targetDescription.Valid()) {
        std::cout << " to " << NameDescriptionToString(targetDescription);
      }

      std::cout << " with";

      for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription->GetDescriptions().begin();
          name!=crossingWaysDescription->GetDescriptions().end();
          ++name) {
        std::cout << " " << NameDescriptionToString(*name);
      }

      std::cout << std::endl;
    }
#endif

    if (startDescription.Valid()) {
      NextLine(lineCount);

      std::cout << "Start at \"" << startDescription->GetDescription() << "\"" << std::endl;
    }

    if (targetDescription.Valid()) {
      NextLine(lineCount);

      std::cout << "Target reached \"" << targetDescription->GetDescription() << "\"" << std::endl;
    }

    if (crossingWaysDescription.Valid() &&
        IsRelevantCrossing(crossingWaysDescription)) {
      std::set<std::string>                          names;
      osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription->GetOriginDesccription();
      osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription->GetTargetDesccription();

      if (originDescription.Valid()) {
        std::string nameString=NameDescriptionToString(originDescription);

        if (!nameString.empty()) {
          names.insert(nameString);
        }
      }

      if (targetDescription.Valid()) {
        std::string nameString=NameDescriptionToString(targetDescription);

        if (!nameString.empty()) {
          names.insert(nameString);
        }
      }

      for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription->GetDescriptions().begin();
          name!=crossingWaysDescription->GetDescriptions().end();
          ++name) {
        std::string nameString=NameDescriptionToString(*name);

        if (!nameString.empty()) {
          names.insert(nameString);
        }
      }

      if (names.size()>1) {
        NextLine(lineCount);

        std::cout << "At crossing ";

        for (std::set<std::string>::const_iterator name=names.begin();
            name!=names.end();
            ++name) {
          if (name!=names.begin()) {
            std::cout << ", ";
          }
          std::cout << *name;
        }

        std::cout << std::endl;
      }

      NextLine(lineCount);

      std::cout << "Turn into way ";
      std::cout << NameDescriptionToString(targetDescription);
      std::cout << std::endl;
    }
    else if (nameChangedDescription.Valid() &&
             IsRelevantNameChange(nameChangedDescription)) {
      NextLine(lineCount);

      std::cout << "Way changes name to ";
      std::cout << NameDescriptionToString(nameChangedDescription->GetTargetDesccription());
      std::cout << std::endl;
    }

    if (lineCount==0) {
      std::cout << std::endl;
    }

#if defined(HTML)
    std::cout << "</td></tr>";
#endif

    prevNode=node;
  }

  router.Close();

  return 0;
}
