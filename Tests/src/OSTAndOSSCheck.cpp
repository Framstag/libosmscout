/*
  DrawTextQt - a test program for libosmscout
  Copyright (C) 2017  Lukas Karas

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

#include <cstdlib>

#include <osmscout/TypeConfig.h>
#include <osmscout/StyleConfig.h>

#include <osmscout/util/File.h>
#include <osmscout/util/CmdLineParsing.h>

using namespace std;

struct Arguments {
  bool help = false;
  bool warningAsError = false;
  std::string ostFile;
  std::string ossFile;
};

int main(int argc, char** argv)
{
  osmscout::CmdLineParser   argParser("MutiDBRouting",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.warningAsError=value;
                      }),
                      "warning-as-error",
                      "Mark all warnings as error",
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.ostFile=value;
                          }),
                          "OST_FILE",
                          "Typedefinition file (*.ost)");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.ossFile=value;
                          }),
                          "OSS_FILE",
                          "Stylesheet file (*.oss)");

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

  size_t errorCount=0;

  osmscout::TypeConfigRef typeConfig=std::make_shared<osmscout::TypeConfig>();
  std::string             ostFilepath=args.ostFile;

  if (typeConfig->LoadFromOSTFile(ostFilepath)) {
    std::cout << "OST file '" << ostFilepath << "' => OK" << std::endl;


    osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);
    std::string             ossFilepath=args.ossFile;

    if (styleConfig->Load(ossFilepath)) {
      if (styleConfig->GetWarnings().empty()) {
        std::cout << "OSS file '" << ossFilepath << "' => OK" << std::endl;
      }else{
        std::cout << "OSS file '" << ossFilepath << "' => WARNINGS" << std::endl;
        if (args.warningAsError){
          errorCount+=styleConfig->GetWarnings().size();
        }
      }
    }
    else {
      std::cerr << "OSS file '" << ossFilepath << "' => ERROR" << std::endl;
      errorCount++;
    }
  }
  else {
    std::cerr << "OST file '" << ostFilepath << "' => ERROR" << std::endl;
    errorCount++;
  }

  if (errorCount>0) {
    return 1;
  }
  else {
    return 0;
  }
}
