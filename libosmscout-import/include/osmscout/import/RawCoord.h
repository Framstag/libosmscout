#ifndef OSMSCOUT_IMPORT_RAWCOORD_H
#define OSMSCOUT_IMPORT_RAWCOORD_H

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

#include <osmscout/GeoCoord.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Representation of a type-less OSM node, just representing a geographic
   * coordinate.
   */
  class RawCoord CLASS_FINAL
  {
  private:
    OSMId    id;    //<! OSM id of the corresponding node
    GeoCoord coord; //<! The actual coordinate

  public:
    RawCoord();

    /**
     * Return the OSM node id
     */
    inline OSMId GetOSMId() const
    {
      return id;
    }

    /**
     * Retutrn the geographic coordinate
     */
    inline const GeoCoord& GetCoord() const
    {
      return coord;
    }

    /**
     * Return true, if both nodes have the same OSM id
     */
    inline bool IsIdentical(const RawCoord& other) const
    {
      return id==other.id;
    }

    /**
     * Return true, if both nodes have the same geographic coordinate
     */
    inline bool IsSame(const RawCoord& other) const
    {
      return coord==other.coord;
    }

    /**
     * return if the nodes are either identicial or the same
     */
    inline bool IsEqual(const RawCoord& other) const
    {
      return id==other.id || coord==other.coord;
    }

    void SetOSMId(OSMId id);

    void SetCoord(const GeoCoord& coord);
    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
    void Write(FileWriter& writer) const;
  };

  typedef std::shared_ptr<RawCoord> RawCoordRef;
}

#endif
