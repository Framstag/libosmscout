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

#ifndef OSMSCOUT_TEST_ALLOCATION_COUNTER_H
#define OSMSCOUT_TEST_ALLOCATION_COUNTER_H

#include <cstddef>

namespace osmscout {
  namespace test {

    /**
     * Return 'true' if the counting allocator is compiled into this test binary.
     *
     * The counting allocator replaces the global operator new and counts every heap block the
     * process allocates, so the difference between two calls is the allocation volume of the code in
     * between. It is not compiled in when the binary is built with a sanitizer, neither
     * AddressSanitizer nor MemorySanitizer: a replacement of operator new would take the allocations
     * away from the sanitizer's own bookkeeping and the sanitizer runtime defines the same operators,
     * which is why the sanitizer configuration excludes the tests that need the counter.
     */
    bool AllocationCounterEnabled();

    /**
     * Number of heap blocks the process allocated since it started. Aligned allocations are not
     * counted. Return zero if the counting allocator is not compiled in.
     */
    size_t GetAllocationCount();
  }
}

#endif
