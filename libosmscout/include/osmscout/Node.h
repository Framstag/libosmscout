#ifndef OSMSCOUT_NODE_H
#define OSMSCOUT_NODE_H

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

#include <vector>

#include <osmscout/FileReader.h>
#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  class Node
  {
  public:
    Id                id;
    TypeId            type;
    double            lon;
    double            lat;
    std::vector<Tag>  tags;

  public:
    inline Node()
    : type(typeIgnore)
    {
      // no code
    }

    bool Read(FileReader& reader);
    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };
}

#endif
