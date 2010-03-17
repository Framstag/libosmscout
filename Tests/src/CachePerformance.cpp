/*
  CachePerformance - a test program for libosmscout
  Copyright (C) 2010  Tim Teulings

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

#include <cstdlib>
#include <iostream>

#include <osmscout/FileScanner.h>
#include <osmscout/Way.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/Util.h>

/**
  Sequentially read the ways.dat file in the current directory to collect
  way ids. Then take a subset of ids randomly and call the Cache via WayDataFile for
  different cache sizes. Measure execution time.

  Call this program repeately to avoid different timing because of OS file caching.
*/

int main(int argc, char* argv[])
{
  std::vector<osmscout::Id> ids;
  std::vector<osmscout::Id> queries;
  std::string               filename="ways.dat";
  size_t                    readerWayCount;

  osmscout::StopClock readerTimer;

  osmscout::FileScanner scanner;

  if (!scanner.Open(filename)) {
    std::cerr << "Cannot open file '" << filename << "'!" << std::endl;
    return 1;
  }

  std::cout << "Start reading files using FileReader..." << std::endl;

  readerWayCount=0;
  while (!scanner.HasError()) {
    osmscout::Way way;

    if (way.Read(scanner)) {
      ids.push_back(way.id);
      readerWayCount++;
    }
  }

  scanner.Close();

  readerTimer.Stop();

  std::cout << "Reading " << readerWayCount << " ways via FileReader took " << readerTimer << std::endl;

  queries.reserve(readerWayCount/10);

  for (size_t i=0; i<readerWayCount/10; i++) {
    queries.push_back(ids[(int)(readerWayCount/10*rand()/(RAND_MAX+1.0))]);
  }

  size_t cacheSize=1;

  for (size_t i=1; i<=7; i++) {
    osmscout::WayDataFile wayDataFile("ways.dat","way.idx",cacheSize);

    if (!wayDataFile.Open(".")) {
      std::cerr << "Cannot open way data file!" << std::endl;
      return 1;
    }

    osmscout::StopClock cacheTimer;

    for (size_t r=0; r<10; r++) {
      for (size_t idx=0; idx<queries.size(); idx++) {
        osmscout::Way way;

        if (!wayDataFile.Get(queries[idx],way)) {
          std::cerr << "Cannot read way with id " << queries[idx] << " from data file!" << std::endl;
        }
      }
    }

    cacheTimer.Stop();

    std::cout << "Reading " << queries.size() << " random ways from data file with cache size " << cacheSize << " took " << cacheTimer << std::endl;

    wayDataFile.Close();

    cacheSize=cacheSize*10;
  }

  return 0;
}
