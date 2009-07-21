#ifndef OSMSCOUT_NODEINDEX_H
#define OSMSCOUT_NODEINDEX_H

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

#include <osmscout/TypeConfig.h>

struct NodeIndexEntry
{
  size_t    interval;
  size_t    offset;
  NodeCount nodeCount;
};

class NodeIndex
{
private:
  struct IndexEntry
  {
    size_t    offset;
    NodeCount nodeCount;
  };

private:
  std::map<size_t,IndexEntry> nodeIndex;
  size_t                      intervalSize;

public:
  bool LoadNodeIndex(const std::string& path);

  size_t GetIntervalSize() const;
  void GetNodeIndexEntries(const std::set<Id>& nodeIds, std::list<NodeIndexEntry>& entries) const;
  void GetNodePagesIndexEntries(const std::set<Page>& pages, std::list<NodeIndexEntry>& entries) const;

  void DumpStatistics();
};

#endif
