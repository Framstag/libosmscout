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

#include <TestAllocationCounter.h>

#include <atomic>
#include <cstdlib>
#include <new>

/**
 * The sanitizer runtimes bring their own replacement of the global operator new and delete, so a
 * replacement here would collide with them at link time: the AddressSanitizer runtime defines them
 * in its C++ runtime, and the MemorySanitizer runtime defines the same set in `msan_new_delete.cpp`
 * (which is always linked when memory instrumentation is on), so the counting allocator is compiled
 * out whenever a sanitizer is in use.
 */
#if defined(__has_feature)
  #if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer)
    #define OSMSCOUT_TEST_SANITIZER_RUNTIME 1
  #endif
#endif

#if defined(__SANITIZE_ADDRESS__)
  #define OSMSCOUT_TEST_SANITIZER_RUNTIME 1
#endif

#if !defined(OSMSCOUT_TEST_SANITIZER_RUNTIME)
  #define OSMSCOUT_TEST_COUNT_ALLOCATIONS 1
#endif

#if defined(OSMSCOUT_TEST_COUNT_ALLOCATIONS)

namespace {

  std::atomic<size_t> allocationCounter{0};

}

void* operator new(std::size_t size)
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  void* memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void* operator new[](std::size_t size)
{
  return ::operator new(size);
}

void* operator new(std::size_t size,
                   const std::nothrow_t&) noexcept
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  return std::malloc(size);
}

void* operator new[](std::size_t size,
                     const std::nothrow_t& tag) noexcept
{
  return ::operator new(size,tag);
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     const std::nothrow_t&) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       const std::nothrow_t&) noexcept
{
  std::free(memory);
}

namespace osmscout {
  namespace test {

    bool AllocationCounterEnabled()
    {
      return true;
    }

    size_t GetAllocationCount()
    {
      return allocationCounter.load(std::memory_order_relaxed);
    }
  }
}

#else

namespace osmscout {
  namespace test {

    bool AllocationCounterEnabled()
    {
      return false;
    }

    size_t GetAllocationCount()
    {
      return 0;
    }
  }
}

#endif
