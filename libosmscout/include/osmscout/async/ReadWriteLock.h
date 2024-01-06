#ifndef LIBOSMSCOUT_READWRITELOCK_H
#define LIBOSMSCOUT_READWRITELOCK_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2024 Lukas Karas

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

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/private/Config.h>

#include <shared_mutex>
#include <mutex>

#ifdef OSMSCOUT_PTHREAD
#include <pthread.h>
#endif

namespace osmscout {

#ifdef OSMSCOUT_PTHREAD

  /** C++ std::shared_mutex may lead to writer starvation with multiple highly concurrent readers,
   * and it doesn't provide API to configure priority. Luckily, with Pthread implementation,
   * we may configure writer preference. It expect that write operations are rare, compared to read ones.
   */
  class OSMSCOUT_API SharedMutex {
  private:
    pthread_rwlock_t rwlock{};

  public:
    SharedMutex();
    SharedMutex(const SharedMutex&) = delete;
    SharedMutex(SharedMutex&&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;
    SharedMutex& operator=(SharedMutex&&) = delete;
    ~SharedMutex();

    void lock();
    void unlock();

    void lock_shared();
    void unlock_shared();
  };

#else

  using SharedMutex = std::shared_mutex;

#endif

  using ReadLock = std::shared_lock<SharedMutex>;
  using WriteLock = std::unique_lock<SharedMutex>;

}

#endif //LIBOSMSCOUT_READWRITELOCK_H
