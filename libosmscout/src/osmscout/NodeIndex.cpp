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

#include <osmscout/NodeIndex.h>

#include <cassert>
#include <fstream>
#include <iostream>

bool NodeIndex::LoadNodeIndex(const std::string& path)
{
  std::ifstream indexFile;
  std::string   file=path+"/"+"node.idx";

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

    indexFile.read((char*)&interval,sizeof(interval)); // The interval
    indexFile.read((char*)&entry.offset,sizeof(entry.offset));     // The offset into the way.dat file
    indexFile.read((char*)&entry.nodeCount,sizeof(entry.nodeCount)); // The number of ways in theinterval

    nodeIndex[interval]=entry;
  }

  if (!indexFile) {
    indexFile.close();
    return false;
  }

  indexFile.close();
  return true;
}

size_t NodeIndex::GetIntervalSize() const
{
  return intervalSize;
}

void NodeIndex::GetNodeIndexEntries(const std::set<Id>& nodeIds,
                                    std::list<NodeIndexEntry>& entries) const
{
  std::set<size_t> intervals;

  for (std::set<Id>::const_iterator node=nodeIds.begin();
       node!=nodeIds.end();
       ++node) {
    intervals.insert(*node/intervalSize);
  }

  for (std::set<size_t>::const_iterator interval=intervals.begin();
       interval!=intervals.end();
       ++interval) {

    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=nodeIndex.find(*interval);

    assert(entry!=nodeIndex.end());

    NodeIndexEntry tmp;

    tmp.interval=entry->first;;
    tmp.offset=entry->second.offset;
    tmp.nodeCount=entry->second.nodeCount;

    entries.push_back(tmp);
  }
}

void NodeIndex::GetNodePagesIndexEntries(const std::set<Page>& pages,
                                         std::list<NodeIndexEntry>& entries) const
{
  for (std::set<size_t>::const_iterator interval=pages.begin();
       interval!=pages.end();
       ++interval) {

    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=nodeIndex.find(*interval);

    assert(entry!=nodeIndex.end());

    NodeIndexEntry tmp;

    tmp.interval=entry->first;;
    tmp.offset=entry->second.offset;
    tmp.nodeCount=entry->second.nodeCount;

    entries.push_back(tmp);
  }
}

void NodeIndex::DumpStatistics()
{
  size_t memory=0;

  memory+=nodeIndex.size()*sizeof(size_t);     // Key
  memory+=nodeIndex.size()*sizeof(IndexEntry); // Value

  std::cout << "Node index size " << nodeIndex.size() << ", memory usage " << memory << std::endl;
}

