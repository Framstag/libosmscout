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
  Check performance of
  * cache insertion
  * cache hit
  * cache miss
*/

/**
  Empty fake data object
  */
struct Data
{
};

static const size_t cacheSize=1000000;

typedef osmscout::Cache<osmscout::Id,Data> Cache;

int main(int argc, char* argv[])
{
  Cache  cache(cacheSize);

  osmscout::StopClock insertTimer;

  std::cout << "Inserting values from " << cacheSize << " to " << 2*cacheSize-1 << " into cache..." << std::endl;

  for (size_t i=cacheSize; i<2*cacheSize; i++) {
    Cache::CacheEntry entry(i,Data());

    Cache::CacheRef ref=cache.SetEntry(entry);
  }

  insertTimer.Stop();

  std::cout << "Cache size: " << cache.GetSize() << std::endl;

  osmscout::StopClock missTimer;

  std::cout << "Searching for entries not in cache..." << std::endl;

  for (size_t i=0; i<cacheSize; i++) {
    Cache::CacheRef entry;

    if (cache.GetEntry(i,entry)) {
      assert(false);
    }
  }

  for (size_t i=2*cacheSize; i<3*cacheSize; i++) {
    Cache::CacheRef entry;

    if (cache.GetEntry(i,entry)) {
      assert(false);
    }
  }

  missTimer.Stop();

  osmscout::StopClock hitTimer;

  std::cout << "Searching for entries in cache..." << std::endl;

  for (size_t t=1; t<=2; t++) {
    for (size_t i=cacheSize; i<2*cacheSize; i++) {
      Cache::CacheRef entry;

      if (!cache.GetEntry(i,entry)) {
        assert(false);
      }
    }
  }

  hitTimer.Stop();

  std::cout << "Insert time: "  << insertTimer << std::endl;
  std::cout << "Miss time: "  << missTimer << std::endl;
  std::cout << "Hit time: "  << hitTimer << std::endl;

  return 0;
}
