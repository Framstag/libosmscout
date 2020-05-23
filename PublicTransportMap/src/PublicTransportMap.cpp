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

#include <iostream>

#include <osmscout/PTRouteDataFile.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/FileScanner.h>

/*
 * Example:
 *   PublicTransportMap ../maps/arnsberg-regbez routemaster_tram DSW21
 */

struct Arguments
{
  bool        help=false;
  std::string databaseDirectory;
  std::string typeName;
  std::string operatorName;
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
      if (!arguments.operatorName.empty()) {
        if (route->GetOperator()!=arguments.operatorName) {
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

void DumpRoutes(const std::list<osmscout::PTRouteRef>& routes)
{
  for (const auto& route : routes) {
    std::cout << route->GetRef() << " - " << route->GetName() << std::endl;

    for (const auto& variant : route->variants) {
      std::cout << "* " << variant.GetName() << std::endl;

      for (const auto& stop : variant.stops) {
        std::cout << "  - " << stop.GetStop().GetName() << std::endl;
      }
    }
  }
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

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Database directory");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.typeName=value;
                          }),
                          "TYPE",
                          "Type of the route");


  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.operatorName=value;
                          }),
                          "OPERATOR",
                          "Name of the operator");

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

  auto typeConfig=std::make_shared<osmscout::TypeConfig>();

  std::cout << "Loading type config..." << std::endl;
  if (!typeConfig->LoadFromDataFile(args.databaseDirectory)) {
    std::cerr << "Cannot load '"  << osmscout::TypeConfig::FILE_TYPES_DAT << "'!" << std::endl;
    return 1;
  }
  std::cout << "Loading type config...done" << std::endl;

  auto routeType=typeConfig->GetTypeInfo(args.typeName);

  if (!routeType) {
    std::cerr << "Cannot find type '"  << args.typeName << "'!" << std::endl;
    return 1;
  }

  std::list<osmscout::PTRouteRef> routes;

  std::cout << "Loading routes..." << std::endl;
  if (!LoadPTRoutes(args,*typeConfig,routeType,routes)) {
    return 1;
  }
  std::cout << "Loading routes...done" << std::endl;

  std::cout << "Found " << routes.size() << " routes" << std::endl;

  routes.sort([](const osmscout::PTRouteRef& a,
                 const osmscout::PTRouteRef& b)->bool {
    return a->GetRef()<b->GetRef();
  });

  DumpRoutes(routes);

  return 0;
}
