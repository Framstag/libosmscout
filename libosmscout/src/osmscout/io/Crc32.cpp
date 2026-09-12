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

#include <osmscout/io/Crc32.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

namespace osmscout {

  namespace {

    constexpr uint32_t kCrc32Polynomial=0xEDB88320U;
    constexpr size_t   kCrc32TableSize=256;
    constexpr int      kCrc32BitsPerByte=8;

    std::array<uint32_t,kCrc32TableSize> BuildCrc32Table()
    {
      std::array<uint32_t,kCrc32TableSize> table{};

      for (uint32_t i=0; i<kCrc32TableSize; i++) {
        uint32_t crc=i;

        for (int j=0; j<kCrc32BitsPerByte; j++) {
          crc=((crc & 1U)!=0U) ? (crc >> 1) ^ kCrc32Polynomial
                               : crc >> 1;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access) index is bounded by the loop
        table[i]=crc;
      }

      return table;
    }
  }

  // NOLINTBEGIN(bugprone-easily-swappable-parameters) natural parameter order for a hash function
  uint32_t Crc32(const void* data,
                 size_t size,
                 uint32_t crc)
  {
    static const std::array<uint32_t,kCrc32TableSize> table=BuildCrc32Table();

    const auto                                        * bytes=static_cast<const uint8_t*>(data);

    crc=~crc;

    for (size_t i=0; i<size; i++) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access, cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-avoid-magic-numbers) index is masked to 0-255
      crc=table[(crc ^ bytes[i]) & 0xFFU] ^ (crc >> 8);
    }

    return ~crc;
  }
  // NOLINTEND(bugprone-easily-swappable-parameters)

  bool ComputeFileCrc32(const std::string& path,
                        uint32_t& crc32Value)
  {
    std::ifstream file(path,
                       std::ios::binary);

    if (!file) {
      return false;
    }

    const size_t      bufferSize=static_cast<size_t>(64)*1024;
    std::vector<char> buffer(bufferSize);
    uint32_t          crc=0;

    while (file) {
      file.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));

      std::streamsize bytesRead=file.gcount();

      if (bytesRead>0) {
        crc=Crc32(buffer.data(),
                  static_cast<size_t>(bytesRead),
                  crc);
      }
    }

    if (file.bad()) {
      return false;
    }

    crc32Value=crc;

    return true;
  }
}
