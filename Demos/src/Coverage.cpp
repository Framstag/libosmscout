/*
  Coverage - a demo program for libosmscout
  Copyright (C) 2018  Tim Teulings

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

#include <osmscout/CoverageIndex.h>

#include <osmscout/util/CmdLineParsing.h>

#include <iostream>

struct Arguments
{
  bool                   help;
  osmscout::GeoCoord     coordinate;
  std::list<std::string> mapPaths;
};

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("Coverage",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.coordinate=value;
                          }),
                          "coordinate",
                          "Coordinate to test");

  argParser.AddPositional(osmscout::CmdLineStringListOption([&args](const std::string& value) {
                            args.mapPaths.push_back(value);
                          }),
                          "mapDb",
                          "List of map db directories to test coverage against");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  for (const auto& indexFile : args.mapPaths) {
    osmscout::CoverageIndex index;

    if (index.Open(indexFile)) {
      bool covered=index.IsCovered(args.coordinate);

      std::cout << (covered ? "+" : "-") << " Is " << (covered ? "" : "NOT ") << "covered by index '" << indexFile << "'" << std::endl;

      index.Close();
    }
    else {
      std::cerr << "! Cannot open index file '" << indexFile << "'" << std::endl;
    };
  }

  return 0;
}

