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

static size_t retryCount = 20;

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

  size_t dataCacheSize=1;
  size_t indexCacheSize=1;

  for (size_t i=1; i<=7; i++) {
    osmscout::WayDataFile wayDataFile("ways.dat","way.idx",dataCacheSize,indexCacheSize);

    if (!wayDataFile.Open(".")) {
      std::cerr << "Cannot open way data file!" << std::endl;
      return 1;
    }

    osmscout::StopClock cacheTimer;

    for (size_t r=1; r<=retryCount; r++) {
      for (size_t i=0; i<queries.size(); i++) {
        size_t        idx=(int)(queries.size()*rand()/(RAND_MAX+1.0));
        osmscout::Way way;

        if (!wayDataFile.Get(queries[idx],way)) {
          std::cerr << "Cannot read way with id " << queries[idx] << " from data file!" << std::endl;
        }
      }
    }

    cacheTimer.Stop();

    osmscout::StopClock cacheMissTimer;

    for (size_t r=1; r<=retryCount; r++) {
      for (size_t i=0; i<queries.size(); i++) {
        osmscout::Way way;

        if (wayDataFile.Get((osmscout::Id)-1,way)) {
          std::cerr << "Unexpecte dsuccessful read from data file!" << std::endl;
        }
      }
    }

    cacheMissTimer.Stop();


    std::cout << "Reading " << retryCount*queries.size() << " random ways from data file with cache size " << dataCacheSize << "," << indexCacheSize << " took " << cacheTimer << std::endl;
    std::cout << "Reading " << retryCount*queries.size() << " misses from data file with cache size " << dataCacheSize << "," << indexCacheSize << " took " << cacheMissTimer << std::endl;

    wayDataFile.Close();

    dataCacheSize=dataCacheSize*10;
    indexCacheSize=indexCacheSize*10;
  }

  return 0;
}
