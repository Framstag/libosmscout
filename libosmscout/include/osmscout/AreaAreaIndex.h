#ifndef OSMSCOUT_AREAAREAINDEX_H
#define OSMSCOUT_AREAAREAINDEX_H

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

#include <map>
#include <set>
#include <vector>

#include <osmscout/StyleConfig.h>

class AreaAreaIndex
{
private:
  struct IndexEntry
  {
    std::vector<FileOffset> dataOffsets;
    FileOffset              children[4];
  };

  typedef std::map<size_t,IndexEntry> IndexLevel;

private:
  std::vector<double>     cellWidth;
  std::vector<double>     cellHeight;
  size_t                  maxLevel;
  std::vector<IndexLevel> index;

public:
  bool Load(const std::string& path);

  void GetOffsets(const StyleConfig& styleConfig,
                  double minlon, double minlat,
                  double maxlon, double maxlat,
                  size_t maxLevel,
                  size_t maxCount,
                  std::set<FileOffset>& offsets) const;

  void DumpStatistics();
};

#endif
