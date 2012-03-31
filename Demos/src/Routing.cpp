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

#include <osmscout/Database.h>
#include <osmscout/Router.h>

//#define POINTS_DEBUG
#define ROUTE_DEBUG
#define NODE_DEBUG

/*
  Examples for the nordrhein-westfalen.osm:

  Long:
  src/Routing ../TravelJinni/ 14332719 138190834 10414977 283372120

  Medium:
  src/Routing ../TravelJinni/ 33879936 388178882 38363871 453298626

  Short:
  src/Routing ../TravelJinni/ 33879936 388178882 24922615 270813923

  Roundabout
  src/Routing ../TravelJinni/ 24998462 271758830 42123520 523731341
*/

static std::string TimeToString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

  return stream.str();
}

static bool IsRelevantCrossing(const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossing,
                               const osmscout::RouteDescription::TurnDescriptionRef& turn)
{
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossing->GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossing->GetTargetDesccription();

  // Roundabout enter/leave is always interesting
  if (crossing->GetType()!=osmscout::RouteDescription::CrossingWaysDescription::normal) {
    return true;
  }

  // If we have to turn, this is important!
  if (crossing->HasMultipleExits()) {
    // If it is only a slight turn and the name stays, we do not mention it
    // TODO: More precisely we should always skip a crossing with multiple exists,
    // if the turn we make is the (obviously) smalest possible for all paths we could go from here.
    if (originDescription.Valid() &&
        targetDescription.Valid() &&
        originDescription->GetName()==targetDescription->GetName() &&
        originDescription->GetRef()==targetDescription->GetRef() &&
        (turn->GetCurve()==osmscout::RouteDescription::TurnDescription::slightlyLeft ||
         turn->GetCurve()==osmscout::RouteDescription::TurnDescription::slightlyRight)) {
      return false;
    }
    else if (turn->GetCurve()!=osmscout::RouteDescription::TurnDescription::straightOn) {
      return true;
    }
  }

  return false;
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

  osmscout::RouteDescription::DescriptionRef description;

  if (node.HasDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC) &&
      node.HasDescription(osmscout::RouteDescription::TURN_DESC)) {
    description=node.GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    osmscout::RouteDescription::CrossingWaysDescriptionRef crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(description.Get());
    description=node.GetDescription(osmscout::RouteDescription::TURN_DESC);
    osmscout::RouteDescription::TurnDescriptionRef turnDescription=dynamic_cast<osmscout::RouteDescription::TurnDescription*>(description.Get());

    if (IsRelevantCrossing(crossingWaysDescription,turnDescription)) {
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
  postprocessors.push_back(new osmscout::RoutePostprocessor::TargetPostprocessor("Target"));
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNamePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNameChangedPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::CrossingWaysPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::TurnPostprocessor());

  osmscout::DatabaseParameter  databaseParameter;
  osmscout::Database           database(databaseParameter);
  osmscout::RoutePostprocessor postprocessor;
  size_t                       roundaboutCrossingCounter=0;

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

#if defined(POINTS_DEBUG)
  std::list<osmscout::Point> points;

  if (!router.TransformRouteDataToPoints(data,points)) {
    std::cerr << "Error during route conversion" << std::endl;
  }

  std::cout << points.size() << " point(s)" << std::endl;
  for (std::list<osmscout::Point>::const_iterator point=points.begin();
      point!=points.end();
      ++point) {
    std::cout << "Point " << point->GetId() << " " << point->GetLat() << "," << point->GetLon() << std::endl;
  }
#endif

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
    osmscout::RouteDescription::DescriptionRef             desc;
    osmscout::RouteDescription::StartDescriptionRef        startDescription;
    osmscout::RouteDescription::TargetDescriptionRef       targetDescription;
    osmscout::RouteDescription::NameDescriptionRef         nameDescription;
    osmscout::RouteDescription::NameChangedDescriptionRef  nameChangedDescription;
    osmscout::RouteDescription::CrossingWaysDescriptionRef crossingWaysDescription;
    osmscout::RouteDescription::TurnDescriptionRef         turnDescription;

    desc=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
    if (desc.Valid()) {
      startDescription=dynamic_cast<osmscout::RouteDescription::StartDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
    if (desc.Valid()) {
      targetDescription=dynamic_cast<osmscout::RouteDescription::TargetDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
    if (desc.Valid()) {
      nameDescription=dynamic_cast<osmscout::RouteDescription::NameDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    if (desc.Valid()) {
      nameChangedDescription=dynamic_cast<osmscout::RouteDescription::NameChangedDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    if (desc.Valid()) {
      crossingWaysDescription=dynamic_cast<osmscout::RouteDescription::CrossingWaysDescription*>(desc.Get());
    }

    desc=node->GetDescription(osmscout::RouteDescription::TURN_DESC);
    if (desc.Valid()) {
      turnDescription=dynamic_cast<osmscout::RouteDescription::TurnDescription*>(desc.Get());
    }

    if (crossingWaysDescription.Valid() &&
        roundaboutCrossingCounter>0 &&
        crossingWaysDescription->GetExitCount()>1) {
      roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
    }

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

#if defined(ROUTE_DEBUG) || defined(NODE_DEBUG)
    NextLine(lineCount);

    std::cout << "# " << node->GetTime() << "h " << std::setw(0) << std::setprecision(3) << node->GetDistance() << "km " << node->GetCurrentNodeId() << " => " << node->GetPathWayId() << "[" << node->GetTargetNodeId() << "]" << std::endl;

    if (startDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << startDescription->GetDebugString() << std::endl;
    }

    if (targetDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << targetDescription->GetDebugString() << std::endl;
    }

    if (nameDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << nameDescription->GetDebugString() << std::endl;
    }

    if (nameChangedDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << nameChangedDescription->GetDebugString() << std::endl;
    }

    if (crossingWaysDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << crossingWaysDescription->GetDebugString() << std::endl;
    }

    if (turnDescription.Valid()) {
      NextLine(lineCount);
      std::cout << "# " << turnDescription->GetDebugString() << std::endl;
    }
#endif

    if (startDescription.Valid()) {
      NextLine(lineCount);

      std::cout << "Start at '" << startDescription->GetDescription() << "'" << std::endl;
    }

    if (targetDescription.Valid()) {
      NextLine(lineCount);

      std::cout << "Target reached '" << targetDescription->GetDescription() << "'" << std::endl;
    }

    if (crossingWaysDescription.Valid() &&
        turnDescription.Valid() &&
        IsRelevantCrossing(crossingWaysDescription,turnDescription)) {
      std::set<std::string>                          names;
      osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription->GetOriginDesccription();
      osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription->GetTargetDesccription();

      if (originDescription.Valid()) {
        std::string nameString=originDescription->GetDescription();

        if (!nameString.empty()) {
          names.insert(nameString);
        }
      }

      if (targetDescription.Valid()) {
        std::string nameString=targetDescription->GetDescription();

        if (!nameString.empty()) {
          names.insert(nameString);
        }
      }

      for (std::list<osmscout::RouteDescription::NameDescriptionRef>::const_iterator name=crossingWaysDescription->GetDescriptions().begin();
          name!=crossingWaysDescription->GetDescriptions().end();
          ++name) {
        std::string nameString=(*name)->GetDescription();

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
          std::cout << "'" << *name << "'";
        }

        std::cout << std::endl;
      }

      NextLine(lineCount);

      if (crossingWaysDescription->GetType()==osmscout::RouteDescription::CrossingWaysDescription::roundaboutEnter) {
        std::cout << "Enter roundabout" << std::endl;
        roundaboutCrossingCounter=1;
      }
      else if (crossingWaysDescription->GetType()==osmscout::RouteDescription::CrossingWaysDescription::roundaboutLeave) {
        std::cout << "Leave roundabout into way '";
        std::cout << targetDescription->GetDescription();
        std::cout << "'";

        std::cout << " (" << roundaboutCrossingCounter-1 << ". crossing)";

        std::cout << std::endl;
        roundaboutCrossingCounter=0;
      }
      else {
        std::string turnCommand="turn";

        if (turnDescription.Valid()) {
          switch (turnDescription->GetCurve()) {
          case osmscout::RouteDescription::TurnDescription::sharpLeft:
            turnCommand="Turn sharp left";
            break;
          case osmscout::RouteDescription::TurnDescription::left:
            turnCommand="Turn left";
            break;
          case osmscout::RouteDescription::TurnDescription::slightlyLeft:
            turnCommand="Turn slightly left";
            break;
          case osmscout::RouteDescription::TurnDescription::straightOn:
            turnCommand="Straight on";
            break;
          case osmscout::RouteDescription::TurnDescription::slightlyRight:
            turnCommand="Turn slightly right";
            break;
          case osmscout::RouteDescription::TurnDescription::right:
            turnCommand="Turn right";
            break;
          case osmscout::RouteDescription::TurnDescription::sharpRight:
            turnCommand="Turn sharp right";
            break;
          }
        }

        std::cout << turnCommand << " into way '";
        std::cout << targetDescription->GetDescription();
        std::cout << "'" << std::endl;
      }
    }
    else if (nameChangedDescription.Valid() &&
             IsRelevantNameChange(nameChangedDescription)) {
      NextLine(lineCount);

      std::cout << "Way changes name to '";
      std::cout << nameChangedDescription->GetTargetDesccription()->GetDescription();
      std::cout << "'" << std::endl;
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
