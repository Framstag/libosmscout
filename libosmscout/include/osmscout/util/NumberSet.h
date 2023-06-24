#ifndef OSMSCOUT_UTIL_NUMBERSET_H
#define OSMSCOUT_UTIL_NUMBERSET_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/lib/CoreImportExport.h>

#include <bitset>
#include <unordered_map>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Util
   *
   */
  class OSMSCOUT_API NumberSet CLASS_FINAL
  {
  private:
    using Bitset = std::bitset<4096>;
    using Map    = std::unordered_map<size_t, Bitset>;

  private:
    Map                                       map;
    size_t                                    count;

    public:
    NumberSet();

    void Set(Id id);
    bool IsSet(Id id) const;
    size_t GetNodeUsedCount() const;

    void Clear();
  };

}

#endif
