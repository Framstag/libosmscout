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

#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/NumberSet.h>
#include <osmscout/util/StopClock.h>

/**
  Generate a number of random potential node ids in the range 0...max(long)
  and check the performance of std::set<unsigned long> against NumberSet.
*/

size_t       ID_COUNT=50000000; // Number of insert/find tests bases on random numbers
osmscout::Id UPPER_LIMIT=100000;//std::numeric_limits<osmscout::Id>::max(); // upper range for test values

int main(int /*argc*/, char* /*argv*/[])
{
  std::vector<osmscout::Id> ids;

  ids.resize(ID_COUNT);

  std::random_device               rd;  //Will be used to obtain a seed for the random number engine
  std::mt19937                     gen(rd()); //Standard mersenne_twister_engine seeded with rd()
  std::uniform_real_distribution<> dis(1, UPPER_LIMIT);

  std::cout << "Generate random ids..." << std::endl;

  for (auto& id : ids) {
    id=(osmscout::Id)dis(gen);
  }

  std::cout << "Inserting into std::set..." << std::endl;

  osmscout::StopClock insertsetTimer;

  std::set<osmscout::Id> set;

  for (auto id : ids) {
    set.insert(id);
  }

  insertsetTimer.Stop();

  std::cout << "Inserting into std::unordered_set..." << std::endl;

  osmscout::StopClock insertusetTimer;

  std::unordered_set<osmscout::Id> uset;

  for (auto id : ids) {
    uset.insert(id);
  }

  insertusetTimer.Stop();

  std::cout << "Inserting into NumberSet..." << std::endl;

  osmscout::StopClock insertnsetTimer;

  osmscout::NumberSet nset;

  for (auto id : ids) {
    nset.Set(id);
  }

  insertnsetTimer.Stop();

  std::cout << "Searching in std::set..." << std::endl;

  osmscout::StopClock stestsetTimer;

  for (auto id : ids) {
    if (set.find(id)==set.end()) {
      std::cerr << "set error!" << std::endl;
    }
  }

  stestsetTimer.Stop();

  std::cout << "Searching in std::unordered_set..." << std::endl;

  osmscout::StopClock stestusetTimer;

  for (auto id : ids) {
    if (uset.find(id)==uset.end()) {
      std::cerr << "unordered_set error!" << std::endl;
    }
  }

  stestusetTimer.Stop();

  std::cout << "Searching in NumberSet..." << std::endl;

  osmscout::StopClock stestnsetTimer;

  for (auto id : ids) {
    if (!nset.IsSet(id)) {
      std::cerr << "NumberSet error!" << std::endl;
    }
  }

  stestnsetTimer.Stop();

  std::cout << "Inserting " << ID_COUNT << " ids into std::set took " << insertsetTimer << std::endl;
  std::cout << "Inserting " << ID_COUNT << " ids into std::unordered_set took " << insertusetTimer << std::endl;
  std::cout << "Inserting " << ID_COUNT << " ids into NumberSet took " << insertnsetTimer << std::endl;
  std::cout << "Testing " << ID_COUNT << " ids in std::set took " << stestsetTimer << std::endl;
  std::cout << "Testing " << ID_COUNT << " ids in std::unordered_set took " << stestusetTimer << std::endl;
  std::cout << "Testing " << ID_COUNT << " ids in NumberSet took " << stestnsetTimer << std::endl;

  return 0;
}
