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
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "frame_pixel_layout.h"

/*
 * The two render entry points hand the same frame to two consumers that do not read the
 * same layout: Bitmap.setPixels reads 0xAARRGGBB int[] elements, and
 * Bitmap.copyPixelsFromBuffer reads the bitmap's own byte order (R,G,B,A on ARGB_8888).
 * This test pins both, because getting it wrong is a colour change that no Kotlin-side
 * test can see — the 2026-09-29 defect wrote the int[] layout into the direct buffer, so
 * the daylight palette's motorway colour #7d7af5 arrived as #f57a7d (red) and the water
 * colour #9acffd as #fdcf9a (orange) on the device.
 */

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
}

TEST_CASE("The two destinations do not share a layout")
{
  uint32_t argbPixels[2] = {0, 0};
  uint8_t rgbaBytes[2 * 4] = {0};

  naviveylin::FrameDestination intDestination{argbPixels,
                                              naviveylin::FrameLayout::ArgbInt};
  naviveylin::FrameDestination byteDestination{rgbaBytes,
                                               naviveylin::FrameLayout::RgbaBytes};

  // The two colours that made the defect visible on the device.
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
