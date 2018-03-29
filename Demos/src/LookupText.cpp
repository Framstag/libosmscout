/*
  LookupText - a demo program for libosmscout
  Copyright (C) 2013  Preet Desai

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
#include <osmscout/TextSearchIndex.h>
#include <osmscout/Database.h>
#include <algorithm>

#include <osmscout/util/CmdLineParsing.h>

struct Arguments
{
  bool        help;
  std::string databaseDirectory;

  Arguments()
    : help(false)
  {
    // no code
  }
};

void printDetails(const osmscout::FeatureValueBuffer& features)
{
  std::cout << "   - type:     " << features.GetType()->GetName() << std::endl;

  for (const auto& featureInstance :features.GetType()->GetFeatures()) {
    if (features.HasFeature(featureInstance.GetIndex())) {
      osmscout::FeatureRef feature=featureInstance.GetFeature();

      if (feature->HasValue() && feature->HasLabel()) {
        osmscout::FeatureValue *value=features.GetValue(featureInstance.GetIndex());
        if (value->GetLabel().empty()) {
          std::cout << "   + feature " << feature->GetName() << std::endl;
        }
        else {
          std::cout << "   + feature " << feature->GetName() << ": " << osmscout::UTF8StringToLocaleString(value->GetLabel()) << std::endl;
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
  osmscout::CmdLineParser   argParser("LookupText",
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
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the database to use");

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

  std::cout << "* Searches are case-sensitive\n"
               "* Displays up to 10 unique text results\n"
               "* Displays up to 5 file offsets for each result\n"
               "* Input at least 3 characters or 'q' to quit\n" << std::endl;


  while(true) {
    std::string searchInput;

    std::cout << std::endl;
    std::cout << "Enter a search term:"<< std::endl;
    std::getline(std::cin,searchInput);

    if(searchInput.size() < 3) {
      if(searchInput.size()==1) {
        if(searchInput[0]=='q' || searchInput[0]=='Q') {
          std::cout << "INFO: Quitting" << std::endl;
          break;
        }
      }
      std::cout << "Input at least 3 characters" << std::endl;
      continue;
    }

    // search using the text input as the query
    osmscout::TextSearchIndex::ResultsMap results;

    textSearch.Search(osmscout::LocaleStringToUTF8String(searchInput),true,true,true,true,results);

    if(results.empty()) {
      std::cout << "No results found." << std::endl;
      continue;
    }

    osmscout::DatabaseParameter databaseParameter;
    osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);
    if (!database->Open(args.databaseDirectory)) {
      std::cerr << "Cannot open database" << std::endl;
      return 1;
    }

    // print out the results
    size_t printCount=0;
    osmscout::TextSearchIndex::ResultsMap::iterator it;
    for(it=results.begin(); it != results.end(); ++it) {
      std::cout << "\"" <<it->first << "\" -> " << std::endl;
      std::vector<osmscout::ObjectFileRef> &refs=it->second;
      std::size_t maxPrintedOffsets=5;
      std::size_t minRefCount=std::min(refs.size(),maxPrintedOffsets);

      for(size_t r=0; r < minRefCount; r++) {
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
      if(refs.size() > 10) {
        std::cout << "... " << (refs.size()-10) << " more offsets";
      }
      std::cout << std::endl;
      printCount++;
      if(printCount == 10) {
        break;
      }
    }

    if(results.size() > 10) {
      std::cout << "... " << results.size()
                << " total unique text results found" << std::endl;
    }
  }

  return 0;
}
