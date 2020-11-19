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
#include <iomanip>

using namespace std;

class StyleConfigAnalyzer: public osmscout::StyleConfig
{
private:
  template <typename Selector>
  bool HasStyle(const std::vector<std::vector<std::list<Selector>>> &styleLookup, size_t typeIndex) const
  {
    assert(styleLookup.size() > typeIndex);
    return !styleLookup[typeIndex].empty();
  }

  template <typename Selector>
  bool HasStyle(const std::vector<std::vector<std::vector<std::list<Selector>>>> &selectors, size_t typeIndex) const
  {
    for (const auto &styleLookup: selectors){
      if (HasStyle(styleLookup, typeIndex)){
        return true;
      }
    }
    return false;
  }

  bool HasStyle(const std::vector<osmscout::TypeInfoSet> &typeSets,
                const osmscout::TypeInfoRef &type) const
  {
    return std::any_of(typeSets.begin(), typeSets.end(),
                       [&type](const auto &typeSet){

      return typeSet.IsSet(type);
    });
  }

public:
  explicit StyleConfigAnalyzer(const osmscout::TypeConfigRef& typeConfig):
    osmscout::StyleConfig(typeConfig)
  {}

  ~StyleConfigAnalyzer() override = default;

  void Analyze() const
  {
    using namespace std::string_literals;

    std::vector<osmscout::TypeInfoRef> areasWithoutStyle;
    std::vector<osmscout::TypeInfoRef> waysWithoutStyle;
    std::vector<osmscout::TypeInfoRef> nodesWithoutStyle;

    size_t maxMagLevel = std::max(std::max(areaTypeSets.size(), wayTypeSets.size()),
                                  nodeTypeSets.size()) -1;

    std::cout << std::setw(35+5) << std::left << "type" << "visible on magnification" << std::endl;
    for (size_t i=0; i<(35+5+10+maxMagLevel+2); i++){
      std::cout << '-';
    }
    std::cout << std::endl;

    auto PrintLevels = [&maxMagLevel](const std::string &objectType,
                                      const std::vector<osmscout::TypeInfoSet> &typeSets,
                                      const osmscout::TypeInfoRef &type){

      std::cout << objectType << " " << std::setw(35) << std::left << type->GetName() << " ";
      size_t minMag=maxMagLevel;
      size_t maxMag=0;
      for (size_t mag=0; mag<=maxMagLevel; mag++){
        osmscout::TypeInfoSet types=typeSets[std::min(mag,typeSets.size()-1)];
        if (types.IsSet(type)) {
          minMag=std::min(mag,minMag);
          maxMag=std::max(mag,maxMag);
        }
      }
      std::cout << std::right
                << std::setw(2) << std::setfill(' ') << minMag << " - "
                << std::setw(2) << std::setfill(' ') << maxMag << " |";
      for (size_t mag=0; mag<=maxMagLevel; mag++){
        osmscout::TypeInfoSet types=typeSets[std::min(mag,typeSets.size()-1)];
        std::cout << (types.IsSet(type) ? "x" : " ");
      }
      std::cout << "|" << std::endl;
    };

    for (const auto &type: typeConfig->GetTypes()) {
      if (type->GetIgnore()) {
        continue;
      }

      if (type->CanBeArea()) {
        if (HasStyle(areaTypeSets, type)) {
          PrintLevels("Area"s, areaTypeSets, type);
        } else {
          areasWithoutStyle.push_back(type);
        }
      }

      if (type->CanBeWay()) {
        if (HasStyle(wayTypeSets, type)) {
          PrintLevels("Way "s, wayTypeSets, type);
        } else {
          waysWithoutStyle.push_back(type);
        }
      }

      if (type->CanBeNode()){
        if (HasStyle(nodeTypeSets, type)) {
          PrintLevels("Node"s, nodeTypeSets, type);
        } else {
          nodesWithoutStyle.push_back(type);
        }
      }
    }

    auto PrintTypes = [](const std::vector<osmscout::TypeInfoRef> &types) {
      for (const auto &type: types){
        std::cout << type->GetName() << " ";
      }
    };

    std::cout << "[I] Area types without style: ";
    PrintTypes(areasWithoutStyle);
    std::cout << std::endl;

    std::cout << "[I] Way types without style: ";
    PrintTypes(waysWithoutStyle);
    std::cout << std::endl;

    std::cout << "[I] Node types without style: ";
    PrintTypes(nodesWithoutStyle);
    std::cout << std::endl;

  }

};

using StyleConfigAnalyzerRef = std::shared_ptr<StyleConfigAnalyzer>;

struct Arguments {
  bool help = false;
  bool warningAsError = false;
  bool analyze = false;
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

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.analyze=value;
                      }),
                      "analyze",
                      "Show detailed analysis of stylesheet",
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


    StyleConfigAnalyzerRef styleConfig=std::make_shared<StyleConfigAnalyzer>(typeConfig);
    std::string            ossFilepath=args.ossFile;

    if (styleConfig->Load(ossFilepath)) {
      if (styleConfig->GetWarnings().empty()) {
        std::cout << "OSS file '" << ossFilepath << "' => OK" << std::endl;
      }else{
        std::cout << "OSS file '" << ossFilepath << "' => WARNINGS" << std::endl;
        if (args.warningAsError){
          errorCount+=styleConfig->GetWarnings().size();
        }
      }

      if (args.analyze){
        std::cout << std::endl;
        styleConfig->Analyze();
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
