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

#include <cstddef>
#include <cstdint>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "frame_pixel_layout.h"

/*
 * The two render entry points hand the same frame to two consumers that do not read the
 * same layout: Bitmap.setPixels reads 0xAARRGGBB int[] elements, and
 * Bitmap.copyPixelsFromBuffer reads the bitmap's own byte order (R,G,B,A on ARGB_8888).
 * This test pins both, because getting it wrong is a colour change that no Kotlin-side
 * test can see — the defect wrote the int[] layout into the direct buffer, so a pixel
 * arrived with red and blue swapped.
 */

namespace {

// Reads a pixel back the way the destination's own consumer reads it: an int[] element is
// a 0xAARRGGBB word, a bitmap buffer is the bytes R,G,B,A. Returning the three colour
// components makes the two layouts comparable, which is exactly the property a channel
// swap breaks.
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

Color ReadArgbPixel(const uint32_t *pixels, size_t index)
{
  uint32_t word = pixels[index];

  return Color{static_cast<uint8_t>((word >> 16) & 0xFFu),
               static_cast<uint8_t>((word >> 8) & 0xFFu),
               static_cast<uint8_t>(word & 0xFFu)};
}

Color ReadRgbaPixel(const uint8_t *bytes, size_t index)
{
  const uint8_t *pixel = bytes + index * 4;

  return Color{pixel[0], pixel[1], pixel[2]};
}

} // namespace

TEST_CASE("The int[] destination writes 0xAARRGGBB words")
{
  std::vector<uint32_t> pixels(2, 0x12345678u);

  naviveylin::FrameDestination destination{pixels.data(),
                                           naviveylin::FrameLayout::ArgbInt};

  destination.Clear(pixels.size());

  REQUIRE(pixels[0] == 0xFF000000u);
  REQUIRE(pixels[1] == 0xFF000000u);

  destination.Write(0, 0x7d, 0x7a, 0xf5); // standard.oss motorwayColor, day palette

  REQUIRE(pixels[0] == 0xFF7D7AF5u);
  REQUIRE(pixels[1] == 0xFF000000u);      // the other pixel is untouched

  Color readBack = ReadArgbPixel(pixels.data(), 0);
  REQUIRE(readBack.r == 0x7d);
  REQUIRE(readBack.g == 0x7a);
  REQUIRE(readBack.b == 0xf5);
}

TEST_CASE("The direct-buffer destination writes the bytes R,G,B,A")
{
  std::vector<uint8_t> bytes(2 * 4, 0xAB);

  naviveylin::FrameDestination destination{bytes.data(),
                                           naviveylin::FrameLayout::RgbaBytes};

  destination.Clear(2);

  for (size_t i = 0; i < 2; i++) {
    REQUIRE(bytes[i * 4 + 0] == 0x00);
    REQUIRE(bytes[i * 4 + 1] == 0x00);
    REQUIRE(bytes[i * 4 + 2] == 0x00);
    REQUIRE(bytes[i * 4 + 3] == 0xFF); // CAIRO_FORMAT_RGB24 carries no alpha
  }

  destination.Write(0, 0x7d, 0x7a, 0xf5);

  REQUIRE(bytes[0] == 0x7d);
  REQUIRE(bytes[1] == 0x7a);
  REQUIRE(bytes[2] == 0xf5);
  REQUIRE(bytes[3] == 0xFF);
  REQUIRE(bytes[4] == 0x00); // the other pixel is untouched

  Color readBack = ReadRgbaPixel(bytes.data(), 0);
  REQUIRE(readBack.r == 0x7d);
  REQUIRE(readBack.g == 0x7a);
  REQUIRE(readBack.b == 0xf5);
}

TEST_CASE("The same colour reads back identically from both destinations")
{
  uint32_t argbPixels[2] = {0, 0};
  uint8_t rgbaBytes[2 * 4] = {0};

  naviveylin::FrameDestination intDestination{argbPixels,
                                              naviveylin::FrameLayout::ArgbInt};
  naviveylin::FrameDestination byteDestination{rgbaBytes,
                                               naviveylin::FrameLayout::RgbaBytes};

  // The two colours that made the channel swap visible: a motorway and a river.
  intDestination.Write(0, 0x7d, 0x7a, 0xf5);
  byteDestination.Write(0, 0x7d, 0x7a, 0xf5);
  intDestination.Write(1, 0x9a, 0xcf, 0xfd);
  byteDestination.Write(1, 0x9a, 0xcf, 0xfd);

  for (size_t i = 0; i < 2; i++) {
    Color fromInt = ReadArgbPixel(argbPixels, i);
    Color fromBytes = ReadRgbaPixel(rgbaBytes, i);

    // Each destination is read in its own layout; if one were written in the other's
    // layout, red and blue would come back swapped here.
    REQUIRE(fromInt.r == fromBytes.r);
    REQUIRE(fromInt.g == fromBytes.g);
    REQUIRE(fromInt.b == fromBytes.b);
  }
}

TEST_CASE("The two destinations do not share a layout")
{
  uint32_t argbPixels[2] = {0, 0};
  uint8_t rgbaBytes[2 * 4] = {0};

  naviveylin::FrameDestination intDestination{argbPixels,
                                              naviveylin::FrameLayout::ArgbInt};
  naviveylin::FrameDestination byteDestination{rgbaBytes,
                                               naviveylin::FrameLayout::RgbaBytes};

  intDestination.Write(0, 0x7d, 0x7a, 0xf5); // motorwayColor
  byteDestination.Write(0, 0x7d, 0x7a, 0xf5);
  intDestination.Write(1, 0x9a, 0xcf, 0xfd); // waterColor
  byteDestination.Write(1, 0x9a, 0xcf, 0xfd);

  REQUIRE(argbPixels[0] == 0xFF7D7AF5u);
  REQUIRE(argbPixels[1] == 0xFF9ACFFDu);

  // The buffer's bytes are R,G,B,A, so the little-endian word it holds is 0xAABBGGRR —
  // the byte-swapped image of the int[] word above. A change that makes the two agree is
  // the defect, not a simplification: the buffer goes to Bitmap.copyPixelsFromBuffer,
  // which reads bytes, while the int[] goes to Bitmap.setPixels, which reads words.
  for (size_t i = 0; i < 2; i++) {
    uint32_t asWord = static_cast<uint32_t>(rgbaBytes[i * 4 + 0]) |
                      (static_cast<uint32_t>(rgbaBytes[i * 4 + 1]) << 8) |
                      (static_cast<uint32_t>(rgbaBytes[i * 4 + 2]) << 16) |
                      (static_cast<uint32_t>(rgbaBytes[i * 4 + 3]) << 24);

    uint32_t expected = (argbPixels[i] & 0xFF00FF00u) |
                        ((argbPixels[i] & 0x00FF0000u) >> 16) |
                        ((argbPixels[i] & 0x000000FFu) << 16);

    REQUIRE(asWord == expected);
    REQUIRE(asWord != argbPixels[i]);
  }
}
