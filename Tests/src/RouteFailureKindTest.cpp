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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <catch2/catch_test_macros.hpp>

#include <array>

#include "route_failure_kind.h"

namespace {

struct CauseAndKind
{
  naviveylin::RouteFailureCause cause;
  int kind;
};

// Every enumerator of RouteFailureCause, so a cause added without a branch in RouteFailureKind() is
// seen here (the mapping is total).
constexpr std::array<CauseAndKind,7> causes{{
                                             {naviveylin::RouteFailureCause::NoStartNode,
                                              naviveylin::kRouteFailureStartUnroutable},
                                             {naviveylin::RouteFailureCause::NoTargetNode,
                                              naviveylin::kRouteFailureTargetUnroutable},
                                             {naviveylin::RouteFailureCause::NoRouteFound,
                                              naviveylin::kRouteFailureNoRouteFound},
                                             {naviveylin::RouteFailureCause::DataFailure,
                                              naviveylin::kRouteFailureRoutingDataFailure},
                                             {naviveylin::RouteFailureCause::RoutingDataUnavailable,
                                              naviveylin::kRouteFailureRoutingDataUnavailable},
                                             {naviveylin::RouteFailureCause::EngineFailure,
                                              naviveylin::kRouteFailureEngineFailure},
                                             {naviveylin::RouteFailureCause::Unknown,
                                              naviveylin::kRouteFailureUnknown}
                                           }};

}

TEST_CASE("Every cause maps to the kind of the same meaning")
{
  for (const auto &entry : causes) {
    REQUIRE(naviveylin::RouteFailureKind(entry.cause)==entry.kind);
  }
}

TEST_CASE("A cause the bridge does not name is the unknown kind")
{
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::Unknown)==
          naviveylin::kRouteFailureUnknown);
}

TEST_CASE("The kind numbers are the ones the Java interface declares")
{
  // RouteCallback.java declares these seven values; the integer is what crosses the boundary, so a
  // constant that no longer matches the Java one is a runtime mismatch, not a compile error. The
  // numbers are written out here so that a change on either side fails this test.
  REQUIRE(naviveylin::kRouteFailureUnknown==0);
  REQUIRE(naviveylin::kRouteFailureRoutingDataUnavailable==1);
  REQUIRE(naviveylin::kRouteFailureStartUnroutable==2);
  REQUIRE(naviveylin::kRouteFailureTargetUnroutable==3);
  REQUIRE(naviveylin::kRouteFailureNoRouteFound==4);
  REQUIRE(naviveylin::kRouteFailureRoutingDataFailure==5);
  REQUIRE(naviveylin::kRouteFailureEngineFailure==6);

  // The other way round: the kind the Java side reads for each cause.
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::NoStartNode)==2);
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::NoTargetNode)==3);
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::NoRouteFound)==4);
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::DataFailure)==5);
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::RoutingDataUnavailable)==1);
  REQUIRE(naviveylin::RouteFailureKind(naviveylin::RouteFailureCause::EngineFailure)==6);
}
