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

#include <osmscout/AreaWayIndex2.h>

#include <osmscout/FileScanner.h>
#include <iostream>
bool AreaWayIndex2::LoadAreaWayIndex(const std::string& path)
{
  FileScanner scanner;
  std::string file=path+"/"+"areaway2.idx";

  if (!scanner.Open(file)) {
    std::cerr << "Cannot open file areaway2.idx" << std::endl;
    return false;
  }

  size_t levels;

  if (!scanner.ReadNumber(levels)) {
    std::cerr << "Cannot read data (1)" << std::endl;
    return false;
  }

  index.resize(levels+1);

  int l=levels;

  while (l>=0) {
    size_t entries;

    if (!scanner.ReadNumber(entries)) {
      std::cerr << "Cannot read data (2)" << std::endl;
      return false;
    }

    for (size_t i=0; i<entries; i++) {
      long       offset;
      IndexEntry entry;

      if (!scanner.GetPos(offset)) {
        std::cerr << "Cannot read data (3)" << std::endl;
        return false;
      }

      if (l!=levels) {
        for (size_t c=0; c<4; c++) {
          if (!scanner.ReadNumber(entry.children[c])) {
            std::cerr << "Cannot read data (4)" << std::endl;
            return false;
          }
        }
      }
      else {
        for (size_t c=0; c<4; c++) {
          entry.children[c]=0;
        }
      }

      size_t offsetCount;

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read data (5)" << std::endl;
        return false;
      }

      entry.offsets.reserve(offsetCount);
      for (size_t c=0; c<offsetCount; c++) {
        long o;

        if (!scanner.ReadNumber(o)) {
          std::cerr << "Cannot read data (6), level " << l << " entry " << i << std::endl;
          return false;
        }

        entry.offsets.push_back(o);
      }

      index[l][offset]=entry;
    }

    l--;
  }

  return !scanner.HasError() && scanner.Close();
}

void AreaWayIndex2::GetOffsets(const StyleConfig& styleConfig,
                               double minlon, double minlat,
                               double maxlon, double maxlat,
                               size_t maxCount,
                               std::set<long>& offsets) const
{
}

void AreaWayIndex2::DumpStatistics()
{
}
