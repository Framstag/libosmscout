/*
  PublicTransportMap - a demo program for libosmscout
  Copyright (C) 2020  Tim Teulings

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

#include <fstream>
#include <iostream>

#include <osmscout/FeatureReader.h>

#include <osmscout/Database.h>
#include <osmscout/PTRouteDataFile.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/FileScanner.h>

/*
 * Example:
 *   PublicTransportMap --operator DSW21 ../maps/arnsberg-regbez routemaster_tram map.svg
 */

struct Arguments
{
  bool        help=false;
  bool        debug=false;
  std::string databaseDirectory;
  std::string typeFilter;
  std::string operatorFilter;
  std::string refFilter;
  std::string mapPath;
};

class Direction
{
private:
  std::string from;
  std::string to;

public:
  Direction()= default;
  Direction(const Direction& direction) = default;

  Direction(const std::string& from,
            const std::string& to)
    : from(from),
      to(to)
  {

  }

  inline const std::string& GetFrom() const
  {
    return from;
  }

  inline const std::string& GetTo() const
  {
    return to;
  }

  inline bool IsReverseOf(const Direction& other) const
  {
    return from==other.to && to==other.from;
  }
};

struct Station
{
  std::string name;

  bool operator==(const Station& rhs) const
  {
    return name==rhs.name;
  }

  bool operator!=(const Station& rhs) const
  {
    return !(rhs==*this);
  }

  bool operator<(const Station& rhs) const
  {
    return name<rhs.name;
  }

  bool operator>(const Station& rhs) const
  {
    return rhs<*this;
  }

  bool operator<=(const Station& rhs) const
  {
    return !(rhs<*this);
  }

  bool operator>=(const Station& rhs) const
  {
    return !(*this<rhs);
  }
};

struct Route
{
  std::string            name;
  std::list<Station>     stations;
  std::vector<Direction> directions;
};

bool LoadPTRoutes(const Arguments& arguments,
                  const osmscout::TypeConfig& typeConfig,
                  const osmscout::TypeInfoRef& routeType,
                  std::list<osmscout::PTRouteRef>& routes)
{
  osmscout::FileScanner scanner;

  try {
    auto filename=osmscout::AppendFileToDir(arguments.databaseDirectory,
                                            osmscout::PTRouteDataFile::PTROUTES_DAT);

    scanner.Open(filename,osmscout::FileScanner::Sequential,true);

    uint32_t routesCount;

    scanner.Read(routesCount);

    for (uint32_t r=1; r<=routesCount; r++) {
      osmscout::PTRouteRef route=std::make_shared<osmscout::PTRoute>();

      route->Read(typeConfig,scanner);

      // Filter by type
      if (route->GetType()!=routeType) {
        continue;
      }

      // Filter by operator
      if (!arguments.operatorFilter.empty()) {
        if (route->GetOperator()!=arguments.operatorFilter) {
          continue;
        }
      }

      if (!arguments.refFilter.empty()) {
        if (route->GetRef()!=arguments.refFilter) {
          continue;
        }
      }

      routes.push_back(route);
    }

    scanner.Close();
  }
  catch (osmscout::IOException& e) {
    std::cerr << "ERROR: " << e.GetDescription() << std::endl;

    scanner.CloseFailsafe();

    return false;
  }

  return true;
}

std::list<Route>::iterator AlreadyExists(std::list<Route>& routes, const Route& route)
{
  std::list<Station> reverseStations=route.stations;

  reverseStations.reverse();

  for (auto routeIter=routes.begin(); routeIter!=routes.end(); ++routeIter) {
    if (routeIter->name==route.name &&
      (routeIter->stations==route.stations || routeIter->stations==reverseStations)) {
      return routeIter;
    }
  }

  return routes.end();
}

std::list<Route> TransformRoutes(osmscout::Database& database,
                                 const std::list<osmscout::PTRouteRef>& orgRoutes)
{
  std::list<Route>                 routes;
  osmscout::NameFeatureLabelReader labelReader(*database.GetTypeConfig());

  for (const auto& route : orgRoutes) {
    for (const auto& variant : route->variants) {
      Route newRoute;
      Direction direction=Direction(variant.GetFrom(),variant.GetTo());

      newRoute.name=route->GetRef();

      for (const auto& stop : variant.stops) {
        Station station;

        if (stop.GetStop().IsNode()) {
          osmscout::NodeRef node;

          if (database.GetNodeByOffset(stop.GetStop().GetFileOffset(),node)) {

            std::string label=labelReader.GetLabel(node->GetFeatureValueBuffer());

            station.name=label;
          }
          else {
            station.name="Unknown";
          }
        }
        else {
          station.name="Unknown";
        }

        newRoute.stations.push_back(station);
      }

      auto existingRouteIter=AlreadyExists(routes,newRoute);

      if (existingRouteIter!=routes.end()) {
        existingRouteIter->directions.push_back(direction);
      }
      else {
        newRoute.directions.push_back(direction);
        routes.push_back(newRoute);
      }
    }
  }

  return routes;
}

void DumpRoutes(const std::list<Route>& routes)
{
  for (const auto& route : routes) {
    std::cout << "* " << route.name << std::endl;
    for (const auto& direction : route.directions) {
     std::cout << "    " << "(" << direction.GetFrom() << " => " << direction.GetTo() << ")" << std::endl;
    }

    for (const auto& station : route.stations) {
      std::cout << "  - " << station.name << std::endl;
    }
  }
}

size_t GetMaxStationCount(const std::list<Route>& routes)
{
  size_t max=0;

  for (const auto& route : routes) {
    max=std::max(max,route.stations.size());
  }

  return max;
}

void WriteSVGHeader(std::ofstream& stream, size_t width, size_t height)
{
  stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
  stream << "<!-- Created by the PublicTransportMap tool, part of libosmscout (http://libosmscout.sf.net) -->"
         << std::endl;
  stream << std::endl;

  stream << "<svg" << std::endl;
  stream << "  xmlns:svg=\"http://www.w3.org/2000/svg\"" << std::endl;
  stream << "  xmlns=\"http://www.w3.org/2000/svg\"" << std::endl;
  stream << "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"" << std::endl;
  stream << "  width=\"" << width << "\"" << std::endl;
  stream << "  height=\"" << height << "\"" << std::endl;
  stream << "  id=\"map\"" << std::endl;
  stream << "  version=\"1.1\">" << std::endl;
  stream << std::endl;
  stream << "  <defs>" << std::endl;
  stream << "    <style type=\"text/css\">" << std::endl;
  stream << "       <![CDATA[" << std::endl;
  stream << "         #refLabel {" << std::endl;
  stream << "           fill: #0000aa;" << std::endl;
  stream << "           font-size: 20px;" << std::endl;
  stream << "         }" << std::endl;
  stream << "         #stationLabel {" << std::endl;
  stream << "           fill: #000000;" << std::endl;
  stream << "           font-size: 15px;" << std::endl;
  stream << "         }" << std::endl;
  stream << "         #directionLabel {" << std::endl;
  stream << "           fill: #000000;" << std::endl;
  stream << "           font-size: 15px;" << std::endl;
  stream << "         }" << std::endl;
  stream << "         #refLine {" << std::endl;
  stream << "           stroke: #0000aa;" << std::endl;
  stream << "           stroke-width: 20px;" << std::endl;
  stream << "           stroke-linecap: round;" << std::endl;
  stream << "         }" << std::endl;
  stream << "         #stationCircle {" << std::endl;
  stream << "           fill: #ffffff;" << std::endl;
  stream << "           stroke: #000000;" << std::endl;
  stream << "         }" << std::endl;
  stream << "       ]]>" << std::endl;
  stream << "    </style>" << std::endl;
  stream << "  </defs>" << std::endl;

  stream << std::endl;
}


void WriteStationList(std::ofstream& stream, const Route& route, size_t x, size_t y)
{
  for (const auto& station : route.stations) {
    stream << "  <circle id=\"stationCircle\" cx=\"" << x << "\" cy=\"" << y << "\" r=\"12\"/>" << std::endl;
    stream << "  <text id=\"stationLabel\" " << "x=\"" << x << "\" y=\"" << y-20 << "\" text-anchor=\"end\" writing-mode=\"tb\" glyph-orientation-vertical=\"270\">" << station.name << "</text>" << std::endl;

    x+=35;
  }
}

void WriteRouteList(std::ofstream& stream, const std::list<Route>& routes)
{
  size_t x=50;
  size_t y=450;

  for (const auto& route : routes) {
    stream << "  <text id=\"refLabel\" " << "x=\"" << x << "\" y=\"" << y-100 << "\">" << route.name << "</text>" << std::endl;

    if (route.directions.size()==1) {
      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-80 << "\">"
             << route.directions[0].GetFrom() << "</text>"
             << std::endl;

      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-60 << "\">"
             << "&#x21d2;" << "</text>"
             << std::endl;

      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-40 << "\">"
             << route.directions[0].GetTo() << "</text>"
             << std::endl;
    }

    if (route.directions.size()==2 &&
        route.directions[0].IsReverseOf(route.directions[1])) {
      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-80 << "\">"
             << route.directions[0].GetFrom() << "</text>"
             << std::endl;

      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-60 << "\">"
             << "&#x21d4;" << "</text>"
             << std::endl;

      stream << "  <text id=\"directionLabel\" " << "x=\"" << x << "\" y=\"" << y-40 << "\">"
             << route.directions[0].GetTo() << "</text>"
             << std::endl;
    }

    if (!route.stations.empty()) {
      stream << "  <line id=\"refLine\" x1=\"" << x+300 << "\" y1=\"" << y-10 << "\" x2=\""
             << x+300+(route.stations.size()-1)*35 << "\" y2=\"" << y-10 << "\"/>" << std::endl;

      WriteStationList(stream,
                       route,
                       x+300,
                       y-10);
    }

    stream << std::endl;

    y+=400;
  }
}

void WriteSVGFooter(std::ofstream& stream)
{
  stream << "</svg>" << std::endl;
}

int main(int argc,
         char* argv[])
{
  osmscout::CmdLineParser  argParser("PublicTransportMap",
                                     argc,
                                     argv);
  std::vector<std::string> helpArgs{"h","help"};
  Arguments                args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.operatorFilter=value;
                      }),
                      "operator",
                      "Name of the operator");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.refFilter=value;
                      }),
                      "ref",
                      "Name of the reference");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Database directory");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.typeFilter=value;
                          }),
                          "TYPE",
                          "Type of the route");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.mapPath=value;
                          }),
                          "MAPPATH",
                          "Path of resulting *.svg file");

  osmscout::CmdLineParseResult cmdLineParseResult=argParser.Parse();

  if (cmdLineParseResult.HasError()) {
    std::cerr << "ERROR: " << cmdLineParseResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  osmscout::log.Debug(args.debug);

  osmscout::DatabaseParameter      databaseParameter;
  osmscout::Database               database(databaseParameter);

  if (!database.Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;
  }

  auto routeType=database.GetTypeConfig()->GetTypeInfo(args.typeFilter);

  if (!routeType) {
    std::cerr << "Cannot find type '" << args.typeFilter << "'!" << std::endl;
    return 1;
  }

  std::list<osmscout::PTRouteRef> ptRoutes;

  std::cout << "Loading routes..." << std::endl;
  if (!LoadPTRoutes(args,*database.GetTypeConfig(),routeType,ptRoutes)) {
    return 1;
  }
  std::cout << "Loading routes...done" << std::endl;

  std::cout << "Found " << ptRoutes.size() << " routes" << std::endl;

  ptRoutes.sort([](const osmscout::PTRouteRef& a,
                   const osmscout::PTRouteRef& b)->bool {
    return a->GetRef()<b->GetRef();
  });

  std::list<Route> routes=TransformRoutes(database,ptRoutes);

  DumpRoutes(routes);

  std::ofstream stream(args.mapPath, std::ios_base::binary|std::ios_base::trunc|std::ios_base::out);

  if (!stream) {
    std::cerr << "Cannot open '" << args.mapPath << "' for writing!" << std::endl;
    return 1;
  }

  size_t maxStationCount=GetMaxStationCount(routes);

  WriteSVGHeader(stream,400+maxStationCount*35,routes.size()*400+2*50);

  WriteRouteList(stream,routes);

  WriteSVGFooter(stream);

  stream.close();
  database.Close();

  return 0;
}
