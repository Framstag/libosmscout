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

#include <osmscoutmap/MapParameter.h>

#include "render_label_bounds.h"

TEST_CASE("A tall buffer bounds by its width")
{
  auto bounds = naviveylin::LabelLineBoundsForBuffer(360.0, 640.0);

  REQUIRE(bounds.fitToWidth == 360.0);
  REQUIRE(bounds.minCharCount == 5);
  REQUIRE(bounds.maxCharCount == 15);
  REQUIRE(bounds.fitToArea == true);
}

TEST_CASE("A wide buffer bounds by its height")
{
  auto bounds = naviveylin::LabelLineBoundsForBuffer(1024.0, 768.0);

  REQUIRE(bounds.fitToWidth == 768.0);
}

TEST_CASE("The decision is not the non-binding library default")
{
  // The library's unconfigured fit-to-width of 8000 px binds no label in a viewport
  // of any phone or desktop size; the render states its own bound instead.
  osmscout::MapParameter defaults;

  REQUIRE(defaults.GetLabelLineFitToWidth() == 8000.0);

  auto tallBounds = naviveylin::LabelLineBoundsForBuffer(360.0, 640.0);
  auto wideBounds = naviveylin::LabelLineBoundsForBuffer(1024.0, 768.0);

  REQUIRE(tallBounds.fitToWidth != defaults.GetLabelLineFitToWidth());
  REQUIRE(wideBounds.fitToWidth != defaults.GetLabelLineFitToWidth());
}
