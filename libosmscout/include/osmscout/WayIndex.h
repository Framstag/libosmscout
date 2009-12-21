#ifndef OSMSCOUT_WAYINDEX_H
#define OSMSCOUT_WAYINDEX_H

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

#include <list>
#include <set>

#include <osmscout/Cache.h>
#include <osmscout/FileScanner.h>

#include <osmscout/TypeConfig.h>

struct WayIndexEntry
{
  size_t    interval;
  size_t    offset;
  size_t    size;
  NodeCount count;
};

class WayIndex
{
private:

  struct IndexEntry
  {
    size_t    offset;
    NodeCount count;
  };
private:
  std::map<size_t,IndexEntry> wayIndex;
  size_t                      intervalSize;
  long                        datSize;

public:
  WayIndex();
  virtual ~WayIndex();

  bool LoadWayIndex(const std::string& path);

  size_t GetIntervalSize() const;
  void GetWayIndexEntries(const std::set<Id>& wayIds, std::list<WayIndexEntry>& entries) const;
  void GetWayPagesIndexEntries(const std::set<Page>& pages,
                               std::list<WayIndexEntry>& entries) const;

  void DumpStatistics();
};

#endif
