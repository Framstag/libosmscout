/*
  ReaderScannerPerformance - a test program for libosmscout
  Copyright (C) 2009  Tim Teulings

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
#include <limits>
#include <vector>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/NumberSet.h>
#include <osmscout/util/StopClock.h>

/**
  Generate a number of random potential node ids in the range 0...max(long)
  and check the performance of std::set<unsigned long> against NumberSet.
*/

#define ID_COUNT 10000000

int main(int /*argc*/, char* /*argv*/[])
{
  std::vector<osmscout::Id> ids;

  ids.resize(ID_COUNT);

  for (size_t i=0; i<ids.size(); i++) {
    ids[i]=(int)(std::numeric_limits<unsigned long>::max()*rand()/(RAND_MAX+1.0));
  }

  osmscout::StopClock insertsetTimer;

  std::set<osmscout::Id> set;

  for (size_t i=0; i<ids.size(); i++) {
    set.insert(ids[i]);
  }

  insertsetTimer.Stop();

  osmscout::StopClock insertnsetTimer;

  osmscout::NumberSet nset;

  for (size_t i=0; i<ids.size(); i++) {
    nset.Insert(ids[i]);
  }

  insertnsetTimer.Stop();


  osmscout::StopClock stestsetTimer;

  for (size_t i=0; i<ids.size(); i++) {
    if (set.find(ids[i])==set.end()) {
      std::cerr << "set error!" << std::endl;
    }
  }

  stestsetTimer.Stop();

  osmscout::StopClock stestnsetTimer;

  for (size_t i=0; i<ids.size(); i++) {
    if (!nset.IsSet(ids[i])) {
      std::cerr << "NumberSet error!" << std::endl;
    }
  }

  stestnsetTimer.Stop();

  std::cout << "Inserting " << ID_COUNT << " ids into std::set took " << insertsetTimer << std::endl;
  std::cout << "Inserting " << ID_COUNT << " ids into NumberSet took " << insertnsetTimer << std::endl;
  std::cout << "Testing " << ID_COUNT << " ids in std::set took " << stestsetTimer << std::endl;
  std::cout << "Testing " << ID_COUNT << " ids in NumberSet took " << stestnsetTimer << std::endl;

  return 0;
}
