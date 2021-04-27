/*
  This source is part of the libosmscout library
  Copyright (C) 2021  Jean-Luc Barriere

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

#ifndef UTF8HELPER_CHARMAP_H
#define UTF8HELPER_CHARMAP_H

#include <cstdint>

namespace utf8helper
{

constexpr int None          = 0x00;     // no category
constexpr int IsSpace       = 0x01;     // charcater is space (breaking or non-breaking)
constexpr int IsBreaker     = 0x02;     // character is breaker
constexpr int IsControl     = 0x04;     // character is control sequence
constexpr int IsModifier    = 0x08;     // character is modifier
constexpr int IsDiacritic   = 0x10;     // character is diacritic
constexpr int IsPunctuation = 0x20;     // character is punctuation

using byte = uint8_t;
using codepoint = uint32_t;             // UTF8 codepoint: unsigned 32 bits

constexpr codepoint NullCodepoint = 0;  // the null codepoint (no character)

struct character {
  const codepoint code;     // the codepoint
  const codepoint upper;    // codepoint for the upper case, else the codepoint
  const codepoint lower;    // codepoint for the lower case, else the codepoint
  const int category;       // 32 bits flags to match by category
  const char* translate;    // translated UTF8 string until 4 bytes max
};

/* character map 1 byte US7 ASCII */
extern const character charmap_us7ascii[];

/* character map 2 bytes C0-DF */
extern const character* pagemap_16[32];
extern const character charmap_c2[];
extern const character charmap_c3[];  // latin-1
extern const character charmap_c4[];  // latin-1
extern const character charmap_c5[];
extern const character charmap_c6[];
extern const character charmap_c7[];
extern const character charmap_c8[];
extern const character charmap_c9[];
extern const character charmap_ca[];
extern const character charmap_cb[];
extern const character charmap_cc[];
extern const character charmap_cd[];
extern const character charmap_ce[];
extern const character charmap_cf[];
extern const character charmap_d0[];
extern const character charmap_d1[];
extern const character charmap_d2[];
extern const character charmap_d3[];
extern const character charmap_d4[];
extern const character charmap_d5[];
extern const character charmap_d6[];

/* character map 3 bytes E1 */
extern const character* pagemap_24_e1[];
extern const character charmap_e1_82[];
extern const character charmap_e1_83[];
extern const character charmap_e1_b8[];
extern const character charmap_e1_b9[];
extern const character charmap_e1_ba[];
extern const character charmap_e1_bb[];
extern const character charmap_e1_bc[];
extern const character charmap_e1_bd[];
extern const character charmap_e1_be[];
extern const character charmap_e1_bf[];

/* character map 3 bytes E2 */
extern const character* pagemap_24_e2[];
extern const character charmap_e2_80[];
extern const character charmap_e2_81[];
extern const character charmap_e2_82[];
extern const character charmap_e2_b4[];

/* character map 4 bytes F090 */
extern const character* pagemap_32_f0_90[];
extern const character charmap_f0_90_92[];
extern const character charmap_f0_90_93[];

/* character map 4 bytes F09E */
extern const character* pagemap_32_f0_9e[];
extern const character charmap_f0_9e_a4[];

}

#endif // UTF8HELPER_CHARMAP_H
