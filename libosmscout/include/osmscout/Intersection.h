#ifndef OSMSCOUT_INTERSECTION_H
#define OSMSCOUT_INTERSECTION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/ObjectRef.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * A Intersection is a node, where multiple routeable ways or areas
   * meet.
   */
  class OSMSCOUT_API Intersection CLASS_FINAL
  {
  private:
    Id                         nodeId;  //!< The id/file offset of the node where the ways meet
    std::vector<ObjectFileRef> objects; //!< The objects that meet at the given node

  public:
    Intersection();

    inline Id GetId() const
    {
      return nodeId;
    }

    inline const std::vector<ObjectFileRef>& GetObjects() const
    {
      return objects;
    }

    bool Read(FileScanner& scanner);
    bool Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
  };

  typedef std::shared_ptr<Intersection> JunctionRef;
}

#endif
