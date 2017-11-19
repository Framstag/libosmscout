/*
  GpxImport - a demo program for libosmscout
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/Import.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/Logger.h>

#include <iostream>

struct Arguments
{
  bool               help;
  std::string        gpxFile;

  Arguments()
      : help(false)
  {
    // no code
  }
};

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("GpxImport",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.gpxFile=value;
                          }),
                          "GPXFILE",
                          "Gpx file for import");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  osmscout::log.Debug(true);

  osmscout::GpxFile gpxFile;

  if (!osmscout::Import::ImportGpx(args.gpxFile, gpxFile)){
    return 1;
  }

  return 0;
}
