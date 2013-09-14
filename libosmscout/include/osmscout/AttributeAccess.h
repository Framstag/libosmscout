#ifndef OSMSCOUT_ATTRIBUTE_ACCESS_H
#define OSMSCOUT_ATTRIBUTE_ACCESS_H

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

#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/Types.h>
#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Progress.h>

namespace osmscout {

  /**
   * A path is defined by the way to be used and the node id of a node on this way
   * which is the target to reach.
   */
  class OSMSCOUT_API AttributeAccess
  {
    enum Access {
      footForward     = 1 << 0,
      footBackward    = 1 << 1,
      bicycleForward  = 1 << 2,
      bicycleBackward = 1 << 3,
      carForward      = 1 << 4,
      carBackward     = 1 << 5,
      onewayForward   = 1 << 6,
      onewayBackward  = 1 << 7
    };

  private:
    uint8_t access;

  public:
    AttributeAccess()
    : access(0)
    {
      // no code
    }

    inline uint8_t GetAccess()
    {
      return access;
    }

    inline bool CanRoute() const
    {
      return access & (footForward|footBackward|bicycleForward|bicycleBackward|carForward|carBackward);
    }

    inline bool CanRouteForward() const
    {
      return access & (footForward|bicycleForward|carForward);
    }

    inline bool CanRouteBackward() const
    {
      return access & (footBackward|bicycleBackward|carBackward);
    }

    inline bool CanRouteFoot() const
    {
      return (access & footForward) &&
             (access & footBackward);
    }

    inline bool CanRouteFootForward() const
    {
      return access & footForward;
    }

    inline bool CanRouteFootBackward() const
    {
      return access & footBackward;
    }

    inline bool CanRouteBicycle() const
    {
      return (access & bicycleForward) &&
             (access & bicycleBackward);
    }

    inline bool CanRouteBicycleForward() const
    {
      return access & bicycleForward;
    }

    inline bool CanRouteBicycleBackward() const
    {
      return access & bicycleBackward;
    }

    inline bool CanRouteCar() const
    {
      return (access & carForward) &&
             (access & carBackward);
    }

    inline bool CanRouteCarForward() const
    {
      return access & carForward;
    }

    inline bool CanRouteCarBackward() const
    {
      return access & carBackward;
    }

    inline bool IsOneway() const
    {
      return access & (onewayForward|onewayBackward);
    }

    inline bool IsOnewayForward() const
    {
      return access & onewayForward;
    }

    inline bool IsOnewayBackward() const
    {
      return access & onewayBackward;
    }

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               TypeId type,
               Id id,
               std::vector<Tag>& tags);

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;

    bool operator==(const AttributeAccess& other) const;
    bool operator!=(const AttributeAccess& other) const;
  };
}

#endif
