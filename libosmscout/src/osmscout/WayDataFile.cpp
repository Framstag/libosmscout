/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscout/WayDataFile.h>

namespace osmscout {

  const char* WayDataFile::WAYS_DAT="ways.dat";
  const char* WayDataFile::WAYS_IDMAP="ways.idmap";

  WayDataFile::WayDataFile(size_t cacheSize)
  : DataFile<Way>(WAYS_DAT,cacheSize)
  {
    // no code
  }
}

