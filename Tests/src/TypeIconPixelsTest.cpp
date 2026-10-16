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

#include <cstdint>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "type_icon_pixels.h"

/*
 * The two pure decisions behind the renderTypeIcon entry point, pinned on the host: the
 * factor the artwork is scaled by, and the word Java receives for one painted pixel.
 *
 * Neither can be covered by a rendering test on this machine (no map database of the
 * format the library expects exists here), and both are arithmetic whose mistakes are
 * invisible in a picture: a wrong scale draws a small or clipped icon, an un-premultiplied
 * mistake draws a black fringe around every anti-aliased edge. Hence the host test.
 */

namespace {

// The square the artwork is scaled to fill: the longer side of the artwork, times the fit
// scale, has to be this fraction of the requested side.
constexpr double FitFractionPerSquare = 0.9;

} // namespace

TEST_CASE("Artwork is scaled so its longer side fills the fit fraction of the square")
{
  // A square artwork: both sides are the longer side.
  double squareScale = naviveylin::TypeIconFitScale(48, 32.0);
  REQUIRE(squareScale == Catch::Approx(1.35));
  REQUIRE(32.0 * squareScale == Catch::Approx(FitFractionPerSquare * 48.0));

  // A wide artwork 64 x 32 in a 48 square: the 64 side fills it, the 32 side does not.
  double wideScale = naviveylin::TypeIconFitScale(48, 64.0);
  REQUIRE(wideScale == Catch::Approx(0.675));
  REQUIRE(64.0 * wideScale == Catch::Approx(FitFractionPerSquare * 48.0));
  REQUIRE(32.0 * wideScale < FitFractionPerSquare * 48.0);

  // A tall artwork 20 x 80 in a 64 square: the 80 side fills it, and a different side and
  // extent give a different factor than the two above.
  double tallScale = naviveylin::TypeIconFitScale(64, 80.0);
  REQUIRE(tallScale == Catch::Approx(0.72));
  REQUIRE(80.0 * tallScale == Catch::Approx(FitFractionPerSquare * 64.0));
  REQUIRE(20.0 * tallScale < FitFractionPerSquare * 64.0);

  // The three factors are three distinct numbers: a helper that ignored either input could
  // not return all three.
  REQUIRE(squareScale != wideScale);
  REQUIRE(wideScale != tallScale);
  REQUIRE(squareScale != tallScale);
}

TEST_CASE("Artwork without an extent has no fit scale")
{
  // No extent to scale by: the entry point reports no drawable artwork instead of a scale.
  REQUIRE(naviveylin::TypeIconFitScale(48, 0.0) == 0.0);
  REQUIRE(naviveylin::TypeIconFitScale(48, -4.0) == 0.0);

  // No square to fit into, either.
  REQUIRE(naviveylin::TypeIconFitScale(0, 32.0) == 0.0);
  REQUIRE(naviveylin::TypeIconFitScale(-8, 32.0) == 0.0);
}

TEST_CASE("A pixel is one 0xAARRGGBB word with its channels in place")
{
  // Opaque, and byte 0 is blue while byte 2 is red: the word's low byte is the surface's
  // first byte, which is the order cairoData[offset + 0 .. 3] is read in.
  REQUIRE(naviveylin::TypeIconArgbWord(0x03, 0x02, 0x01, 0xFF) == 0xFF010203u);

  // A pixel nothing was painted into is fully transparent, with no colour left in it.
  REQUIRE(naviveylin::TypeIconArgbWord(0x00, 0x00, 0x00, 0x00) == 0x00000000u);
  REQUIRE(naviveylin::TypeIconArgbWord(0x7f, 0x7f, 0x7f, 0x00) == 0x00000000u);
}

TEST_CASE("An opaque pixel is handed over untouched")
{
  REQUIRE(naviveylin::TypeIconArgbWord(0x40, 0x80, 0xC0, 0xFF) == 0xFFC08040u);
}

TEST_CASE("A partially transparent pixel comes back un-premultiplied")
{
  /*
   * Cairo stores 0x80 alpha red as the premultiplied b=0, g=0, r=127 (a fully red source
   * channel of 253 premultiplied by 128/255 truncates to 127; the exact colour a backend
   * produced is not the point here, the arithmetic is).
   *
   * 127 * 255 / 128 = 32385 / 128 = 253 (truncated from 253.0078), so the channel comes
   * back as 0xFD: the un-premultiplied colour, which is what Android's ARGB_8888 bitmap
   * needs. Handing the premultiplied value over would deliver 0x80 7F0000 and darken the
   * edge, which is the defect this function exists to prevent.
   */
  REQUIRE(naviveylin::TypeIconArgbWord(0x00, 0x00, 127, 128) == 0x80FD0000u);

  REQUIRE(naviveylin::TypeIconArgbWord(0x00, 0x00, 127, 128) != 0x807F0000u); // premultiplied hand-over
  REQUIRE(naviveylin::TypeIconArgbWord(0x00, 0x00, 127, 128) != 0x80FF0000u); // saturated instead of divided

  // A premultiplied channel cannot exceed the alpha it was premultiplied by, so the clamp
  // is a guard rather than a path the surface produces; it keeps the result a byte when a
  // caller passes one that does.
  REQUIRE(naviveylin::TypeIconArgbWord(0x00, 0x00, 255, 1) == 0x01FF0000u);

  // The alpha itself travels unchanged: it is the one channel the surface did not scale.
  REQUIRE((naviveylin::TypeIconArgbWord(0x00, 0x00, 127, 128) >> 24) == 0x80u);
}
