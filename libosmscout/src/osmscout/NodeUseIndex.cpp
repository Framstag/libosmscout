/*
  TravelJinni - Openstreetmap offline viewer
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

#include <osmscout/NodeUseIndex.h>

#include <cassert>
#include <fstream>
#include <iostream>

bool NodeUseIndex::LoadNodeUseIndex(const std::string& path)
{
  std::ifstream indexFile;
  std::string   file=path+"/"+"nodeuse.idx";

  indexFile.open(file.c_str(),std::ios::in|std::ios::binary);

  if (!indexFile) {
    return false;
  }

  size_t intervalCount;

  indexFile.read((char*)&intervalSize,sizeof(intervalSize)); // Size of interval
  indexFile.read((char*)&intervalCount,sizeof(intervalCount)); // Number of intervals

  std::cout << intervalCount << " entries..." << std::endl;

  for (size_t i=0; i<intervalCount; i++) {
    IndexEntry entry;
    size_t     interval;

    indexFile.read((char*)&interval,sizeof(interval));         // The interval
    indexFile.read((char*)&entry.offset,sizeof(entry.offset)); // The offset into the way.dat file
    indexFile.read((char*)&entry.count,sizeof(entry.count));   // The number of entries in the interval

    nodeUseIndex[interval]=entry;
  }

  if (!indexFile) {
    indexFile.close();
    return false;
  }

  indexFile.close();
  return true;
}

size_t NodeUseIndex::GetIntervalSize() const
{
  return intervalSize;
}

void NodeUseIndex::GetNodeIndexEntries(const std::set<Id>& ids,
                                       std::list<NodeUseIndexEntry>& entries) const
{
  std::set<size_t> intervals;

  for (std::set<Id>::const_iterator node=ids.begin();
       node!=ids.end();
       ++node) {
    intervals.insert(*node/intervalSize);
  }

  for (std::set<size_t>::const_iterator interval=intervals.begin();
       interval!=intervals.end();
       ++interval) {
    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=nodeUseIndex.find(*interval);

    if (entry!=nodeUseIndex.end()) {
      NodeUseIndexEntry tmp;

      tmp.interval=entry->first;;
      tmp.offset=entry->second.offset;
      tmp.count=entry->second.count;

      entries.push_back(tmp);
    }
  }
}

void NodeUseIndex::GetNodePagesIndexEntries(const std::set<Page>& pages,
                                            std::list<NodeUseIndexEntry>& entries) const
{
  for (std::set<size_t>::const_iterator interval=pages.begin();
       interval!=pages.end();
       ++interval) {

    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=nodeUseIndex.find(*interval);

    assert(entry!=nodeUseIndex.end());

    NodeUseIndexEntry tmp;

    tmp.interval=entry->first;;
    tmp.offset=entry->second.offset;
    tmp.count=entry->second.count;

    entries.push_back(tmp);
  }
}

void NodeUseIndex::DumpStatistics()
{
  size_t memory=0;

  memory+=nodeUseIndex.size()*sizeof(size_t);     // Key
  memory+=nodeUseIndex.size()*sizeof(IndexEntry); // Value

  std::cout << "Node index use size " << nodeUseIndex.size() << ", memory usage " << memory << std::endl;
}

