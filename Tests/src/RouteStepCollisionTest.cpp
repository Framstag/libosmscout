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

#include <catch2/catch_test_macros.hpp>

#include <osmscoutclient/routing/RouteStep.h>
#include <osmscoutclientqt/RouteStep.h>

#include <type_traits>

namespace osmscout {

  namespace {

    // Inside namespace osmscout the unqualified name is the Qt client's route step. The new model
    // is not reachable without its namespace, so a translation unit that includes both headers
    // cannot silently bind to the wrong type (spec: route-step-model - the new model does not
    // shadow the Qt client's route step). Were the new type named osmscout::RouteStep as well,
    // this alias would be ambiguous and the translation unit would not compile.
    using UnqualifiedRouteStep=RouteStep;

    static_assert(std::is_same_v<UnqualifiedRouteStep, osmscout::RouteStep>);
    static_assert(!std::is_same_v<UnqualifiedRouteStep, osmscout::client::RouteStep>);

    static_assert(!std::is_same_v<osmscout::RouteStep, osmscout::client::RouteStep>);
  }
}

TEST_CASE("The new route step model and the Qt route step are distinct types")
{
  osmscout::RouteStep         qtStep;
  osmscout::client::RouteStep modelStep;

  // Both types keep their own identity member, so a translation unit that uses both must name the
  // one it means.
  qtStep.type="turn-left";
  modelStep.kind=osmscout::client::RouteStep::Kind::turn;

  REQUIRE(qtStep.GetDistance().AsMeter()==0.0);
  REQUIRE(modelStep.GetDistance().AsMeter()==0.0);
  REQUIRE(qtStep.type==QString("turn-left"));
  REQUIRE(osmscout::client::RouteStep::Kind::turn==modelStep.kind);
}
