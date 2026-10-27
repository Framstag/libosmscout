#ifndef OSMSCOUT_CLIENT_ROUTING_ROUTESTEPBUILDER_H
#define OSMSCOUT_CLIENT_ROUTING_ROUTESTEPBUILDER_H

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/routing/RouteStep.h>

#include <osmscout/routing/RouteDescription.h>

#include <list>

namespace osmscout::client {

  /**
   * \ingroup Routing
   *
   * Produces route steps for a whole route and for the manoeuvre ahead of a position.
   *
   * It satisfies the `RouteInstructionAgent` contract: it is default-constructible and copyable,
   * and the step it produces reports the distance of its node from the route start through
   * `RouteStep::GetDistance()`. Beyond it, the step ahead carries the remaining distance and
   * time to its manoeuvre and, when the following manoeuvre is close enough to be announced, the
   * following manoeuvre as a full step in `nextNext`.
   */
  class OSMSCOUT_CLIENT_API RouteStepBuilder CLASS_FINAL
  {
  public:
    /**
     * Distance between two manoeuvres up to which the following one is announced.
     */
    static constexpr double NextNextWindowMeters=200.0;

    /**
     * How far ahead of the position the builder looks for the manoeuvre and its follower.
     */
    static constexpr double CollectWindowMeters=2*NextNextWindowMeters;

  public:
    RouteStepBuilder() = default;
    RouteStepBuilder(const RouteStepBuilder& other) = default;
    RouteStepBuilder(RouteStepBuilder && other) noexcept = default;
    ~RouteStepBuilder() = default;

    RouteStepBuilder& operator=(const RouteStepBuilder& other) = default;
    RouteStepBuilder& operator=(RouteStepBuilder&& other) noexcept = default;

    /**
     * The steps of the given range, in route order, carrying absolute values only.
     */
    std::list<RouteStep> GenerateRouteInstructions(const RouteDescription::NodeIterator& first,
                                                   const RouteDescription::NodeIterator& last) const;

    /**
     * The step of the manoeuvre ahead of the position, extended with its progress.
     *
     * @param previous the route node the position is at or behind
     * @param last the end of the route's node range
     * @param coord the current position
     */
    RouteStep GenerateNextRouteInstruction(const RouteDescription::NodeIterator& previous,
                                           const RouteDescription::NodeIterator& last,
                                           const GeoCoord& coord) const;

    /**
     * As above, with the progress the position has along its current route segment.
     *
     * @param abscissa fraction of `previous -> next node` the position lies at, 0 when unknown
     */
    RouteStep GenerateNextRouteInstruction(const RouteDescription::NodeIterator& previous,
                                           const RouteDescription::NodeIterator& last,
                                           const GeoCoord& coord,
                                           double abscissa) const;
  };
} // namespace osmscout::client

#endif
