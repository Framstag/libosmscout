#ifndef OSMSCOUT_ROUTENODE_H
#define OSMSCOUT_ROUTENODE_H

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

#include <vector>

#include <osmscout/ObjectRef.h>
#include <osmscout/Path.h>
#include <osmscout/Types.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * A route node is the representation of a node in the routing graph.
   */
  class OSMSCOUT_API RouteNode : public Referencable
  {
  public:
    static const uint8_t hasAccess            = 1 << 0; //! We do have access rights to this way/area
    static const uint8_t usableByFoot         = 1 << 1; //! This path can be traveled by foot
    static const uint8_t usableByBicycle      = 1 << 2; //! This path can be traveled by bicycle
    static const uint8_t usableByCar          = 1 << 3; //! This path can be traveled by car

    /**
     * Exclude regarding use of paths. You cannot use the path with the index "targetPath" if you come
     * from the way with the id "sourceWay".
     */
    struct OSMSCOUT_API Exclude
    {
      ObjectFileRef source;       //! The source way
      uint32_t      targetIndex; //! The index of the target path
    };

    /**
     * A single path that starts at the given route node. A path contains a number of information
     * that are relevant for the router.
     */
    struct OSMSCOUT_API Path
    {
      FileOffset      offset;      //! File Offset of the  targeting route node
      uint32_t        objectIndex; //! The index of the way to use from this route node to the target route node
      TypeId          type;        //! The type of the way
      uint8_t         maxSpeed;    //! Maximum speed allowed on the way
      uint8_t         grade;       //! Quality of road/track 1 (good)...5 (bad)
      //uint8_t         bearing;     //! Encoded initial and final bearing of this path
      uint8_t         flags;       //! Certain flags
      double          distance;    //! Distance from the current route node to the target route node
      double          lat;         //! Latitude of the target node
      double          lon;         //! Longitude of the target node

      inline bool HasAccess() const
      {
        return (flags & hasAccess) != 0;
      }
    };

  public:
    Id                         id;         //! Id of the route node, equal the id of the underlying node
    FileOffset                 fileOffset; //! FileOffset of the route node
    std::vector<ObjectFileRef> objects;    //! List of objects (ways, areas) that cross this route node
    std::vector<Path>          paths;      //! List of paths that can in principle be used from this node
    std::vector<Exclude>       excludes;   //! List of potential excludes regarding use of paths

    inline Id GetId() const
    {
      return id;
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    uint32_t AddObject(const ObjectFileRef& object);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<RouteNode> RouteNodeRef;
}

#endif
