/*
  LookupText - a test program for libosmscout
  Copyright (C) 2022  Lukas Karas

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

#include <algorithm>
#include <iostream>

#include <osmscout/db/TextSearchIndex.h>
#include <osmscout/db/Database.h>

#include <osmscout/cli/CmdLineParsing.h>

struct Arguments
{
  bool        help=false;
  std::string databaseDirectory;
  std::string lookupPhrase;
  int         expectedResults=-1;
};

void printDetails(const osmscout::FeatureValueBuffer& features)
{
  std::cout << "   - type:     " << features.GetType()->GetName() << std::endl;

  for (const auto& featureInstance :features.GetType()->GetFeatures()) {
    if (features.HasFeature(featureInstance.GetIndex())) {
      osmscout::FeatureRef feature=featureInstance.GetFeature();

      if (feature->HasValue() && feature->HasLabel()) {
        osmscout::FeatureValue *value=features.GetValue(featureInstance.GetIndex());
        if (value->GetLabel(osmscout::Locale(), 0).empty()) {
          std::cout << "   + feature " << feature->GetName() << std::endl;
        }
        else {
          std::cout << "   + feature " << feature->GetName() << ": "
                    << osmscout::UTF8StringToLocaleString(value->GetLabel(osmscout::Locale(), 0))
                    << std::endl;
        }
      }
      else {
        std::cout << "   + feature " << feature->GetName() << std::endl;
      }
    }
  }
}

int main (int argc, char *argv[])
{
  using namespace std::string_literals;

  osmscout::CmdLineParser   argParser("TextLookupTest",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineUIntOption([&args](unsigned int value) {
                        args.expectedResults=value;
                      }),
                      "expected-results",
                      "Count of expected results. Test is terminated with 1 when result count differs.",
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the db to use");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.lookupPhrase=value;
                          }),
                          "LOOKUP_PHRASE",
                          "Phrase to lookup");

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

  // load text data files
  osmscout::TextSearchIndex textSearch;

  if(!textSearch.Load(args.databaseDirectory)) {
    std::cout << "ERROR: Failed to load text files!"
                 "(are you sure you passed the right path?)"
              << std::endl;
    return -1;
  }

  // search using the text input as the query
  osmscout::TextSearchIndex::ResultsMap results;

  textSearch.Search(osmscout::LocaleStringToUTF8String(args.lookupPhrase),true,true,true,true,true,results);

  if(results.empty()) {
    std::cout << "No results found." << std::endl;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);
  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open db" << std::endl;
    return 1;
  }

  // print out the results
  osmscout::TextSearchIndex::ResultsMap::iterator it;
  for(it=results.begin(); it != results.end(); ++it) {
    std::cout << "\"" <<it->first << "\" -> " << std::endl;
    std::vector<osmscout::ObjectFileRef> &refs=it->second;

    for(size_t r=0; r < refs.size(); r++) {
      if(refs[r].GetType() == osmscout::refNode) {
        std::cout << " * N:" << refs[r].GetFileOffset() << std::endl;
        osmscout::NodeRef node;
        if (database->GetNodeByOffset(refs[r].GetFileOffset(), node)){
          printDetails(node->GetFeatureValueBuffer());
        }
      }
      else if(refs[r].GetType() == osmscout::refWay) {
        std::cout << " * W:" << refs[r].GetFileOffset() << std::endl;
        osmscout::WayRef way;
        if (database->GetWayByOffset(refs[r].GetFileOffset(), way)){
          printDetails(way->GetFeatureValueBuffer());
        }
      }
      else if(refs[r].GetType() == osmscout::refArea) {
        std::cout << " * A:" << refs[r].GetFileOffset() << std::endl;
        osmscout::AreaRef area;
        if (database->GetAreaByOffset(refs[r].GetFileOffset(), area)){
          printDetails(area->GetFeatureValueBuffer());
        }
      }
    }

    std::cout << std::endl;
  }

  if (args.expectedResults >= 0 && size_t(args.expectedResults) != results.size()) {
    std::cerr << "Number of results (" << results.size() << ") differs from expected (" << args.expectedResults << ")" << std::endl;
    return 1;
  }
  return 0;
}
