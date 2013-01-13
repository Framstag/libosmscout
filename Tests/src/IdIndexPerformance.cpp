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

#include <osmscout/NumericIndex.h>
#include <osmscout/Node.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/StopClock.h>

/**
  Sequentially read the ways.dat file in the current directory to collect
  way ids. Then take a subset of ids randomly and call the IdIndex.
  Measure execution time.

  Call this program repeately to avoid different timing because of OS file caching.
*/

#define QUERY_COUNT 1000000

int main(int argc, char* argv[])
{
  std::vector<osmscout::Id> ids;
  std::vector<osmscout::Id> queries;
  std::string               filename="nodes.dat";
  unsigned long             filesize=0;
  size_t                    readerNodeCount;

  if (!osmscout::GetFileSize(filename,filesize)) {
    std::cerr << "Cannot get file size of file '" << filename << "'!" << std::endl;
    return 1;
  }

  osmscout::StopClock readerTimer;

  osmscout::FileScanner scanner;

  if (!scanner.Open(filename,osmscout::FileScanner::Sequential,true)) {
    std::cerr << "Cannot open file '" << filename << "'!" << std::endl;
    return 1;
  }

  std::cout << "Start reading files using FileScanner..." << std::endl;

  readerNodeCount=0;
  while (!scanner.HasError()) {
    osmscout::Node node;

    if (node.Read(scanner)) {
      ids.push_back(node.GetId());
      readerNodeCount++;
    }
  }

  scanner.Close();

  readerTimer.Stop();

  std::cout << "Reading " << readerNodeCount << " nodes via FileReader took " << readerTimer << std::endl;

  queries.reserve(QUERY_COUNT);

  for (size_t i=0; i<QUERY_COUNT; i++) {
    queries.push_back(ids[(int)(QUERY_COUNT*rand()/(RAND_MAX+1.0))]);
  }

  osmscout::NumericIndex<osmscout::Id> nodeIndex("node.idx",1000);

  if (!nodeIndex.Open(".",osmscout::FileScanner::FastRandom,true)) {
    std::cerr << "Cannot open way index file!" << std::endl;
    return 1;
  }

  std::cout << "Starting numeric index test..." << std::endl;

  osmscout::StopClock indexTimer;

  for (size_t i=0; i<queries.size(); i++) {
    std::vector<osmscout::Id>         ids;
    std::vector<osmscout::FileOffset> offsets;

    ids.push_back(queries[i]);

    nodeIndex.GetOffsets(ids,offsets);

    if (offsets.size()!=1) {
      std::cerr << "Cannot read way id " << queries[i] << " from index!" << std::endl;
    }
  }

  indexTimer.Stop();

  std::cout << "Reading " << queries.size() << " random way ids from index took " << indexTimer << std::endl;

  return 0;
}
