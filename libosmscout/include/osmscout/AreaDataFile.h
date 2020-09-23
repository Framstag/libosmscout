#ifndef OSMSCOUT_AREADATAFILE_H
#define OSMSCOUT_AREADATAFILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <memory>

#include <osmscout/Area.h>
#include <osmscout/DataFile.h>

namespace osmscout {
  /**
    \ingroup Database
    Abstraction for getting cached access to the 'ways.dat' file.
    */
  class OSMSCOUT_API AreaDataFile : public DataFile<Area>
  {
  public:
    static const char* const AREAS_DAT;
    static const char* const AREAS_IDMAP;

  public:
    explicit AreaDataFile(size_t cacheSize);

    // disable copy and move
    AreaDataFile(const AreaDataFile&) = delete;
    AreaDataFile(AreaDataFile&&) = delete;
    AreaDataFile& operator=(const AreaDataFile&) = delete;
    AreaDataFile& operator=(AreaDataFile&&) = delete;

    ~AreaDataFile() override = default;
  };

  using AreaDataFileRef = std::shared_ptr<AreaDataFile>;
}

#endif
