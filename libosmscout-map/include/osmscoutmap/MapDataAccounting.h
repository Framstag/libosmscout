#ifndef OSMSCOUT_MAP_MAPDATAACCOUNTING_H
#define OSMSCOUT_MAP_MAPDATAACCOUNTING_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <cstddef>

#include <osmscout/system/Compiler.h>

#include <osmscoutmap/MapImportExport.h>

namespace osmscout {

  /**
   * \ingroup MapDataBudget
   *
   * Accounts the content of the map data caches in units that do not depend on the kind of the
   * cached entries, so that the content of caches holding different kinds of objects can be added
   * up and compared.
   *
   * An entry count alone is not comparable: a cached area carries a list of points and a list of
   * precomputed segment bounding boxes per ring, a cached node carries a single coordinate. The
   * weights below therefore relate the kinds to each other, and the byte figure of a weight unit
   * turns the accounted total into a memory figure that a client can configure and report.
   */
  class OSMSCOUT_MAP_API MapDataAccounting CLASS_FINAL
  {
  public:
    /**
     * The content of one or more caches, as counts per kind of cached entry.
     */
    struct Cost CLASS_FINAL
    {
      size_t nodeCount=0;       //!< Number of cached nodes
      size_t wayCount=0;        //!< Number of cached ways, including optimized ways
      size_t areaCount=0;       //!< Number of cached areas, including optimized areas
      size_t routeCount=0;      //!< Number of cached routes
      size_t indexEntryCount=0; //!< Number of cached index entries, index cells or index pages

      /**
       * Return 'true' if no entry of any kind is accounted
       */
      bool IsEmpty() const
      {
        return nodeCount==0 &&
               wayCount==0 &&
               areaCount==0 &&
               routeCount==0 &&
               indexEntryCount==0;
      }

      /**
       * Return the number of accounted entries over all kinds
       */
      size_t GetEntryCount() const;
    };

    /**
     * Weight of one cached node. A node carries its coordinate and its feature values.
     */
    static constexpr size_t weightPerNode=1;

    /**
     * Weight of one cached way. A way carries its feature values and the points of a line.
     */
    static constexpr size_t weightPerWay=3;

    /**
     * Weight of one cached area. An area carries its feature values, the points of its rings and the
     * precomputed bounding box of every segment, which dominates the size of an area object.
     */
    static constexpr size_t weightPerArea=30;

    /**
     * Weight of one cached route.
     */
    static constexpr size_t weightPerRoute=3;

    /**
     * Weight of one cached index entry.
     */
    static constexpr size_t weightPerIndexEntry=1;

    /**
     * Number of bytes one weight unit stands for. It is a figure derived from the object layouts
     * above and is used to express and to report a budget in memory units.
     */
    static constexpr size_t bytesPerWeight=128;

  public:
    /**
     * Return the accounted weight of the given content
     */
    static size_t GetWeight(const Cost& cost);

    /**
     * Return the number of bytes the given weight stands for
     */
    static size_t GetBytes(size_t weight);

    /**
     * Return the smallest weight that stands for at least the given number of bytes. A non-zero
     * number of bytes is accounted as at least one weight unit, so that a non-empty budget can
     * always be satisfied by a single small entry.
     */
    static size_t GetWeightForBytes(size_t bytes);
  };

  /**
   * \defgroup MapDataBudget Classes for bounding the memory of the map data caches
   */
}

#endif
