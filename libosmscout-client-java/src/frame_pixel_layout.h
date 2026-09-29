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

/*
 * frame_pixel_layout.h — the pixel layouts of the JNI render entry points
 * (NaviVeylin change "reduce-render-peak-memory", design D1b).
 *
 * The two destinations of one render do NOT share a layout:
 *
 *   * FrameLayout::ArgbInt — one 0xAARRGGBB word per pixel, which is what
 *     Bitmap.setPixels(int[]) takes. The allocating entry point
 *     (renderWithRouteAndPois) hands Java an int[] in this layout.
 *   * FrameLayout::RgbaBytes — four bytes per pixel, R,G,B,A, which is the byte order
 *     Bitmap.copyPixelsFromBuffer reads out of an ARGB_8888 bitmap (the NDK names that
 *     format ANDROID_BITMAP_FORMAT_RGBA_8888), i.e. the little-endian word 0xAABBGGRR.
 *     The buffer-taking entry point (renderInto) writes this into the caller's direct
 *     buffer.
 *
 * Writing one layout into the other destination is a red/blue channel swap, not a
 * rounding difference. It shipped once: the buffer path wrote the int[] layout, so
 * motorways rendered red (#7d7af5 became #f57a7d) and rivers orange (#9acffd became
 * #fdcf9a) on the device on 2026-09-29. FrameDestination is therefore the only place a
 * frame pixel is written, and each entry point states the layout its destination needs.
 *
 * Kept dependency-free (no JNI, no libosmscout includes) so it is unit-tested on the
 * host — see Tests/src/FramePixelLayoutTest.cpp.
 */
#ifndef NAVIVEYLIN_FRAME_PIXEL_LAYOUT_H
#define NAVIVEYLIN_FRAME_PIXEL_LAYOUT_H

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace naviveylin {

enum class FrameLayout {
  ArgbInt,  // one 0xAARRGGBB word per pixel, for an int[] / SetIntArrayRegion
  RgbaBytes // four bytes per pixel, R,G,B,A — a Bitmap's own storage
};

struct FrameDestination {
  void *data;
  FrameLayout layout;

  // Fills the whole frame with opaque black before the render, so an area the style leaves
  // unpainted is black exactly as the allocating path left it. Written per layout, because
  // "opaque black" is not the same byte sequence in the two of them.
  inline void Clear(size_t pixelCount) const
  {
    if (layout == FrameLayout::ArgbInt) {
      std::fill(static_cast<uint32_t *>(data),
                static_cast<uint32_t *>(data) + pixelCount,
                0xFF000000u);
    } else {
      uint8_t *bytes = static_cast<uint8_t *>(data);

      std::fill(bytes, bytes + pixelCount * 4, static_cast<uint8_t>(0x00));

      for (size_t i = 0; i < pixelCount; i++) {
        bytes[i * 4 + 3] = 0xFF;
      }
    }
  }

  inline void Write(size_t index, uint8_t r, uint8_t g, uint8_t b) const
  {
    if (layout == FrameLayout::ArgbInt) {
      static_cast<uint32_t *>(data)[index] =
          0xFF000000u | (static_cast<uint32_t>(r) << 16) |
          (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    } else {
      // Cairo's own order is B,G,R,X, which is neither destination's layout: the swap
      // happens here, once, into the layout this destination states.
      uint8_t *bytes = static_cast<uint8_t *>(data) + index * 4;

      bytes[0] = r;
      bytes[1] = g;
      bytes[2] = b;
      bytes[3] = 0xFF; // CAIRO_FORMAT_RGB24 carries no alpha: the frame is opaque
    }
  }
};

} // namespace naviveylin

#endif
