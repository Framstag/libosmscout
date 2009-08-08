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

#include <osmscout/WayIndex.h>

#include <cassert>
#include <iostream>

#include <osmscout/FileReader.h>
#include <osmscout/Util.h>

WayIndex::WayIndex()
{
  // no code
}

WayIndex::~WayIndex()
{
  // no code
}

bool WayIndex::LoadWayIndex(const std::string& path)
{
  FileReader  reader;
  std::string file=path+"/"+"way.idx";

  if (!GetFileSize(path+"/"+"ways.dat",datSize)) {
    return false;
  }

  if (!reader.Open(file)|| !reader.ReadFileToBuffer()) {
    return false;
  }

  size_t intervalCount;

  reader.ReadNumber(intervalSize);  // Size of interval
  reader.ReadNumber(intervalCount); // Number of intervals

  std::cout << intervalCount << " entries of page size " << intervalSize << "..." << std::endl;

  for (size_t i=0; i<intervalCount; i++) {
    IndexEntry entry;
    size_t     interval;

    reader.ReadNumber(interval);     // The interval
    reader.ReadNumber(entry.offset); // The offset into the way.dat file
    reader.ReadNumber(entry.count);  // The number of ways in the interval

    wayIndex[interval]=entry;
  }

  return !reader.HasError() && reader.Close();
}

size_t WayIndex::GetIntervalSize() const
{
  return intervalSize;
}

void WayIndex::GetWayIndexEntries(const std::set<Id>& wayIds, std::list<WayIndexEntry>& entries) const
{
  std::set<size_t> intervals;

  for (std::set<Id>::const_iterator way=wayIds.begin();
       way!=wayIds.end();
       ++way) {
    intervals.insert(*way/intervalSize);
  }

  for (std::set<size_t>::const_iterator interval=intervals.begin();
       interval!=intervals.end();
       ++interval) {
    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=wayIndex.find(*interval);

    assert(entry!=wayIndex.end());

    WayIndexEntry tmp;

    tmp.interval=entry->first;;
    tmp.offset=entry->second.offset;
    tmp.count=entry->second.count;

    entry++;

    if (entry!=wayIndex.end()) {
      tmp.size=entry->second.offset-tmp.offset;
    }
    else {
      tmp.size=datSize-tmp.offset;
    }

    entries.push_back(tmp);
  }
}

void WayIndex::GetWayPagesIndexEntries(const std::set<Page>& pages,
                                       std::list<WayIndexEntry>& entries) const
{
  for (std::set<size_t>::const_iterator page=pages.begin();
       page!=pages.end();
       ++page) {
    std::map<size_t,IndexEntry>::const_iterator entry;

    entry=wayIndex.find(*page);

    assert(entry!=wayIndex.end());

    WayIndexEntry tmp;

    tmp.interval=entry->first;;
    tmp.offset=entry->second.offset;
    tmp.count=entry->second.count;

    entry++;

    if (entry!=wayIndex.end()) {
      tmp.size=entry->second.offset-tmp.offset;
    }
    else {
      tmp.size=datSize-tmp.offset;
    }

    entries.push_back(tmp);
  }
}

void WayIndex::DumpStatistics()
{
  size_t memory=0;

  memory+=wayIndex.size()*sizeof(size_t);     // Key
  memory+=wayIndex.size()*sizeof(IndexEntry); // Value

  std::cout << "Way index size " << wayIndex.size() << ", memory usage " << memory << std::endl;
}


