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
#include <vector>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/StopClock.h>

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
  size_t              value;
  std::vector<size_t> value2;
};

struct Data2
{
  size_t              value;
  std::vector<size_t> value2;

public:
  Data2()
  {
  }

private:
  Data2(Data2& /*other*/)
  {
  }

  void operator=(Data2& /*other*/)
  {
  }

};

static const size_t cacheSize=2000000;

typedef osmscout::Cache<osmscout::Id,Data>     DataCache;

bool TestData()
{
  std::cout << "*** Caching of struct ***" << std::endl;

  DataCache cache(cacheSize);

  std::cout << "Inserting values into cache..." << std::endl;

  osmscout::StopClock insertTimer;

  for (size_t i=cacheSize; i<2*cacheSize; i++) {
    Data data;
    data.value=i;
    data.value2.resize(10,i);

    DataCache::CacheEntry entry(i,data);

    DataCache::CacheRef ref(cache.SetEntry(entry));
  }

  insertTimer.Stop();

  if (cache.GetSize()!=cacheSize){
    return false;
  }

  std::cout << "Updating values  in cache..." << std::endl;

  osmscout::StopClock updateTimer;

  for (size_t i=cacheSize; i<2*cacheSize; i++) {
    DataCache::CacheEntry entry(i);

    entry.value.value=i;
    entry.value.value2.resize(10,i);

    DataCache::CacheRef ref(cache.SetEntry(entry));
  }

  updateTimer.Stop();

  if (cache.GetSize()!=cacheSize){
    return false;
  }

  std::cout << "Searching for entries not in cache..." << std::endl;

  osmscout::StopClock missTimer;

  for (size_t i=0; i<cacheSize; i++) {
    DataCache::CacheRef entry;

    if (cache.GetEntry(i,entry)) {
      return false;
    }
  }

  for (size_t i=2*cacheSize; i<3*cacheSize; i++) {
    DataCache::CacheRef entry;

    if (cache.GetEntry(i,entry)) {
      return false;
    }
  }

  missTimer.Stop();

  std::cout << "Searching for entries in cache..." << std::endl;

  osmscout::StopClock hitTimer;

  for (size_t t=1; t<=2; t++) {
    for (size_t i=cacheSize; i<2*cacheSize; i++) {
      DataCache::CacheRef entry;

      if (!cache.GetEntry(i,entry)) {
        return false;
      }
    }
  }

  hitTimer.Stop();

  std::cout << "Copying entries from cache..." << std::endl;

  osmscout::StopClock copyTimer;

  for (size_t t=1; t<=2; t++) {
    for (size_t i=cacheSize; i<2*cacheSize; i++) {
      DataCache::CacheRef entry;

      if (!cache.GetEntry(i,entry)) {
        return false;
      }

      Data data=entry->value;
    }
  }

  copyTimer.Stop();

  std::cout << "Insert time: "  << insertTimer << std::endl;
  std::cout << "Update time: "  << updateTimer << std::endl;
  std::cout << "Miss time: "  << missTimer << std::endl;
  std::cout << "Hit time: "  << hitTimer << std::endl;
  std::cout << "Copy time: "  << copyTimer << std::endl;

  return true;
}

int main(int /*argc*/, char* /*argv*/[])
{
  return TestData() ? 0:1;
}
