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

  /*
  file=path+"/"+"way2.idx";

  FileScanner scanner;
  size_t      ways;
  size_t      lastLevelPageStart;

  if (!scanner.Open(file)) {
    return false;
  }

  scanner.ReadNumber(levels);
  scanner.ReadNumber(indexPageSize);
  scanner.Read(ways);
  scanner.Read(lastLevelPageStart);

  std::cout << "way2.idx: " << levels << " " << indexPageSize << " " << ways << " " << lastLevelPageStart << std::endl;

  size_t levelEntries;

  levelEntries=ways/indexPageSize;

  if (ways%indexPageSize!=0) {
    levelEntries++;
  }

  size_t sio=0;
  size_t poo=0;

  std::cout << levelEntries << " entries in first level" << std::endl;

  scanner.SetPos(lastLevelPageStart);

  for (size_t i=0; i<levelEntries; i++) {
    Index2Entry entry;
    size_t si;
    size_t po;

    scanner.ReadNumber(si);
    scanner.ReadNumber(po);

    //std::cout << si << " " << po << std::endl;

    sio+=si;
    poo+=po;

    //std::cout << sio << " " << poo << std::endl;

    entry.startId=sio;
    entry.fileOffset=poo;

    root.push_back(entry);
  }

  for (size_t i=1; i<=levels-1; i++) {
    leafs.push_back(PageCache(1000000));
  }

  return !scanner.HasError() && scanner.Close();*/
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

/*
bool WayIndex::GetWayIndexEntries(const std::set<Id>& wayIds,
                                  std::vector<long>& offsets) const
{
  offsets.reserve(wayIds.size());
  offsets.clear();

  if (wayIds.size()==0) {
    return true;
  }

  for (std::set<Id>::const_iterator way=wayIds.begin();
       way!=wayIds.end();
       ++way) {
    size_t r=0;

    //std::cout << "Way id " << *way << std::endl;

    while (r+1<root.size() && root[r+1].startId<=*way) {
      r++;
    }

    if (r<root.size()) {
      //std::cout << "Way id " << *way <<" => " << r << " " << root[r].fileOffset << std::endl;

      size_t startId=root[r].startId;
      long   offset=root[r].fileOffset;

      for (size_t level=0; level<levels-1; level++) {
        PageCache::CacheRef cacheRef;

        if (!leafs[level].GetEntry(startId,cacheRef)) {
          PageCache::CacheEntry cacheEntry(startId);

          cacheRef=leafs[level].SetEntry(cacheEntry);

          //std::cout << "Loading " << level << " " << startId << " " << offset << std::endl;

          if (!scanner.IsOpen() &&
              !scanner.Open("way2.idx")) {
            return false;
          }

          scanner.SetPos(offset);

          cacheRef->value.reserve(indexPageSize);

          size_t j=0;
          Index2Entry entry;

          entry.startId=0;
          entry.fileOffset=0;

          while (j<indexPageSize && !scanner.HasError()) {
            size_t cidx;
            size_t coff;

            scanner.ReadNumber(coff);
            scanner.ReadNumber(cidx);

            entry.fileOffset+=coff;
            entry.startId+=cidx;

            //std::cout << j << " " << entry.startId << " " << entry.fileOffset << std::endl;

            cacheRef->value.push_back(entry);

            j++;
          }

          assert(level<cacheRef->value.size());
        }

        size_t i=0;
        while (i+1<cacheRef->value.size() &&
               cacheRef->value[i+1].startId<=*way) {
          i++;
        }

        if (i<cacheRef->value.size()) {
          startId=cacheRef->value[i].startId;
          offset=cacheRef->value[i].fileOffset;
          //std::cout << " Way id " << *way <<" => " << i << " " << startId << " " << offset << std::endl;
        }
        else {
          std::cerr << "Way id " << *way << " not found in sub page index!" << std::endl;
        }
      }

      if (*way==startId) {
        offsets.push_back(offset);
      }
      else {
        std::cerr << "Way id " << *way << " not found in sub index!" << std::endl;
      }
      //std::cout << "=> Way id " << *way <<" => " << startId << " " << offset << std::endl;
    }
    else {
      std::cerr << "Way id " << *way << " not found in root index!" << std::endl;
    }
  }

  return true;
}*/

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


