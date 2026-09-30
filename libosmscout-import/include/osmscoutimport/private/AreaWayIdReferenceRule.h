#ifndef OSMSCOUT_IMPORT_AREAWAYIDREFERENCERULE_H
#define OSMSCOUT_IMPORT_AREAWAYIDREFERENCERULE_H

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

#include <cstddef>
#include <unordered_set>
#include <vector>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/Point.h>

#include <osmscoutimport/ImportImportExport.h>

namespace osmscout {

  /**
   * \ingroup Import
   *
   * Decides which node ids keep their serial in the optimized area and way data.
   *
   * An id keeps its serial when more than one routable ring or way references it:
   * a single reference can be resolved without the id, a shared id cannot.
   *
   * The distinctness is per ring and per way, not global: an id that one ring
   * references twice counts as referenced once, because that ring can still be
   * resolved, and the same holds for a way that visits a node twice. A circular
   * way is the exception - the id it returns to is forced to count as referenced
   * at least twice, so that the way stays recognizable as circular.
   *
   * Only the caller decides which objects are routable; an object that is not fed
   * to this rule contributes nothing, and ids it references are then treated as
   * referenced by nothing.
   */
  class OSMSCOUT_IMPORT_API AreaWayIdReferenceRule CLASS_FINAL
  {
  private:
    std::unordered_set<Id> referencedIds;
    std::unordered_set<Id> usedAtLeastTwiceIds;

    //! Scratch buffer for the distinct ids of one object. Reused for every
    //! object so that no container is allocated per ring or per way.
    std::vector<Id> distinctIds;

    size_t CollectDistinctIds(const std::vector<Point>& nodes);

  public:
    AreaWayIdReferenceRule() = default;

    /**
     * Reserves room for the given number of referenced ids. It is a hint from a
     * count the caller has already observed, not a contract.
     */
    void ReserveReferencedIds(size_t count);

    /**
     * Feeds one routable object (a ring or a way). Returns the number of distinct
     * id references the object contributed.
     */
    size_t AddObject(const std::vector<Point>& nodes);

    /**
     * Forces the given id to keep its serial. Used for the id a circular way
     * returns to, which has to stay so that the way stays recognizable as
     * circular, even when no other object references it.
     */
    void ForceSerial(Id id);

    /**
     * Returns true when the given node id keeps its serial.
     */
    bool KeepsSerial(Id id) const;

    /**
     * Drops the membership set of every referenced id. The decision is complete
     * once both inputs have been fed; the set is the largest one and is not needed
     * afterwards.
     */
    void DiscardReferencedIds();

    //! Number of node ids referenced by the objects fed so far.
    size_t GetReferencedIdCount() const;

    //! Number of node ids referenced by more than one of them.
    size_t GetUsedAtLeastTwiceCount() const;
  };

  // Methods

  inline size_t AreaWayIdReferenceRule::GetReferencedIdCount() const
  {
    return referencedIds.size();
  }

  inline size_t AreaWayIdReferenceRule::GetUsedAtLeastTwiceCount() const
  {
    return usedAtLeastTwiceIds.size();
  }

  inline bool AreaWayIdReferenceRule::KeepsSerial(Id id) const
  {
    return usedAtLeastTwiceIds.contains(id);
  }

  inline void AreaWayIdReferenceRule::DiscardReferencedIds()
  {
    referencedIds.clear();
  }
}

#endif // OSMSCOUT_IMPORT_AREAWAYIDREFERENCERULE_H
