#ifndef OSMSCOUT_NODEDATAFILE_H
#define OSMSCOUT_NODEDATAFILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/Node.h>

#include <osmscout/io/DataFile.h>

namespace osmscout {
  /**
    \ingroup Database
    Abstraction for getting cached access to the 'nodes.dat' file.
    */
  class OSMSCOUT_API NodeDataFile : public DataFile<Node>
  {
  public:
    static const char* const NODES_DAT;
    static const char* const NODES_IDMAP;

  public:
    explicit NodeDataFile(size_t cacheSize);

    // disable copy and move
    NodeDataFile(const NodeDataFile&) = delete;
    NodeDataFile(NodeDataFile&&) = delete;
    NodeDataFile& operator=(const NodeDataFile&) = delete;
    NodeDataFile& operator=(NodeDataFile&&) = delete;

    ~NodeDataFile() override = default;
  };

  using NodeDataFileRef = std::shared_ptr<NodeDataFile>;
}

#endif
