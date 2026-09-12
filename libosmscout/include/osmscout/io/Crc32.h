#ifndef OSMSCOUT_IO_CRC32_H
#define OSMSCOUT_IO_CRC32_H

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

#include <cstddef>
#include <cstdint>
#include <string>

#include <osmscout/lib/CoreImportExport.h>

namespace osmscout {

  /**
   * Computes the CRC-32 (IEEE 802.3, polynomial 0x04C11DB7, reflected
   * representation 0xEDB88320) of the given data.
   *
   * The result is bit-identical to zlib's crc32() for the same input, so
   * checksums written by the import tool can be verified with zlib on the
   * client side. The initial value is 0; pass the result of a previous call
   * as crc to chain multiple buffers, exactly like zlib's crc32().
   */
  OSMSCOUT_API uint32_t Crc32(const void* data,
                              size_t size,
                              uint32_t crc=0);

  /**
   * Computes the CRC-32 of the given file.
   *
   * @param path
   *    Full path of the file
   * @param crc32Value
   *    On success, the CRC-32 of the file content
   * @return
   *    True on success, else false
   */
  OSMSCOUT_API bool ComputeFileCrc32(const std::string& path,
                                     uint32_t& crc32Value);
}

#endif //OSMSCOUT_IO_CRC32_H
