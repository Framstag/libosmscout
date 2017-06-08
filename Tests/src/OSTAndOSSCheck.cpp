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

using namespace std;

int main(int /*argc*/, char** /*argv*/)
{
  const std::list<std::string> ostFiles={"map.ost"};
  const std::list<std::string> ossFiles={"standard.oss",
                                         "winter-sports.oss",
                                         "boundaries.oss",
                                         "railways.oss",
                                         "motorways.oss"};

  char*        testsTopDirEnv=getenv("TESTS_TOP_DIR");

  if (testsTopDirEnv==NULL) {
    std::cerr << "Expected environment variable 'TESTS_TOP_DIR' not set" << std::endl;
    // CMake-based tests would fail, if we do not exit here
    return 1;
  }

  std::string   testsTopDir=testsTopDirEnv;

  if (testsTopDir.empty()) {
    std::cerr << "Environment variable 'TESTS_TOP_DIR' is empty" << std::endl;
    return 77;
  }

  if (!osmscout::IsDirectory(testsTopDir)) {
    std::cerr << "Environment variable 'TESTS_TOP_DIR' does not point to directory" << std::endl;
    return 77;
  }

  std::string   stylesheetDir=osmscout::AppendFileToDir(testsTopDir,"../stylesheets");

  if (!osmscout::IsDirectory(stylesheetDir)) {
    std::cerr << "Calculated stylesheet directory does not point to directory" << std::endl;
    return 77;
  }

  std::cout << "Stylesheet directory: '" << stylesheetDir << "'"  << std::endl;

  size_t errorCount=0;

  for (const auto& ostFile : ostFiles) {
    osmscout::TypeConfigRef typeConfig=std::make_shared<osmscout::TypeConfig>();
    std::string             ostFilepath=osmscout::AppendFileToDir(stylesheetDir,ostFile);

    if (typeConfig->LoadFromOSTFile(ostFilepath)) {
      std::cout << "OST file '" << ostFilepath << "' => OK" << std::endl;

      for (const auto& ossFile : ossFiles) {
        osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);
        std::string             ossFilepath=osmscout::AppendFileToDir(stylesheetDir,ossFile);

        if (styleConfig->Load(ossFilepath)) {
          std::cout << "OSS file '" << ossFilepath << "' => OK" << std::endl;
        }
        else {
          std::cerr << "OSS file '" << ossFilepath << "' => ERROR" << std::endl;
          errorCount++;
        }
      }
    }
    else {
      std::cerr << "OST file '" << ostFilepath << "' => ERROR" << std::endl;
      errorCount++;
    }
  }

  if (errorCount>0) {
    return 1;
  }
  else {
    return 0;
  }
}
