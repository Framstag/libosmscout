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
 * type_icon_pixels.h — the two pure decisions of the renderTypeIcon entry point
 * (openspec change "client-java-type-icon-entry-point").
 *
 * The entry point draws one object type's stylesheet artwork into a sizePx x sizePx square
 * and hands Java one 0xAARRGGBB word per pixel. Two steps of that are plain arithmetic
 * with no database, style or Cairo surface behind them, so they live here and are covered
 * by a host test (Tests/src/TypeIconPixelsTest.cpp):
 *
 *   * how far artwork of a given extent is scaled up to fill the requested square, and
 *   * how a pixel of the backend's own surface becomes the word Java receives.
 *
 * Kept dependency-free (no JNI, no libosmscout includes) so the host test includes it
 * alone.
 */
#ifndef NAVIVEYLIN_TYPE_ICON_PIXELS_H
#define NAVIVEYLIN_TYPE_ICON_PIXELS_H

#include <algorithm>
#include <cstdint>

namespace naviveylin {

/*
 * Fraction of the requested square an icon's artwork fills, leaving a margin around it.
 * The single spelling of that number: the entry point has no copy of its own to drift
 * from, it scales through TypeIconFitScale().
 */
inline constexpr double TypeIconFitFraction = 0.9;

/*
 * The factor that scales artwork whose longer side measures `extent` so that it fills
 * TypeIconFitFraction of a `side` x `side` square.
 *
 * Zero when there is no extent to scale by — a side that is not positive, or a symbol or
 * image without one — which the caller reports as "this entry has no drawable artwork"
 * rather than as a scale.
 */
inline double TypeIconFitScale(int side, double extent)
{
  if (side <= 0 || !(extent > 0.0)) {
    return 0.0;
  }

  return (static_cast<double>(side) * TypeIconFitFraction) / extent;
}

/*
 * One pixel of the backend surface, read in that surface's own byte order (B,G,R,A on the
 * platforms this bridge builds for), turned into the word Java receives: 0xAARRGGBB with
 * straight, non-premultiplied channels.
 *
 * The surface is CAIRO_FORMAT_ARGB32, which stores premultiplied channels, while the
 * destination — Android's ARGB_8888 — is not premultiplied. Handing the premultiplied
 * value over would darken every partially transparent pixel by its own alpha: an
 * anti-aliased icon edge would arrive black-fringed instead of in the colour the
 * stylesheet drew. Alpha is therefore divided back out here, once, for a pixel that is
 * neither fully transparent nor fully opaque.
 *
 * The parameters are named in the surface's byte order, so a caller reads bytes 0..3 into
 * them as they come; which byte is which channel is the platform's, not this function's.
 */
inline uint32_t TypeIconArgbWord(uint8_t b, uint8_t g, uint8_t r, uint8_t a)
{
  if (a == 0) {
    // Nothing was painted here: the premultiplied channels of a transparent pixel carry
    // no colour, and the caller shows 0x00000000 for it.
    return 0u;
  }

  if (a != 255) {
    r = static_cast<uint8_t>(std::min<unsigned int>(255u, (static_cast<unsigned int>(r) * 255u) / a));
    g = static_cast<uint8_t>(std::min<unsigned int>(255u, (static_cast<unsigned int>(g) * 255u) / a));
    b = static_cast<uint8_t>(std::min<unsigned int>(255u, (static_cast<unsigned int>(b) * 255u) / a));
  }

  return (static_cast<uint32_t>(a) << 24) |
         (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) |
         static_cast<uint32_t>(b);
}

} // namespace naviveylin

#endif // NAVIVEYLIN_TYPE_ICON_PIXELS_H
