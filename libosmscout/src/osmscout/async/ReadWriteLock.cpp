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

#include <osmscout/private/Config.h>

#include <osmscout/async/ReadWriteLock.h>

#include <cassert>

namespace osmscout {

#ifdef OSMSCOUT_PTHREAD

  SharedMutex::SharedMutex()
  {
    pthread_rwlockattr_t attr{};
    [[maybe_unused]] int res = pthread_rwlockattr_init(&attr);
    assert(res==0);

#if defined __USE_UNIX98 || defined __USE_XOPEN2K
    res = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    assert(res==0);
#endif

    pthread_rwlock_t rwlock{};
    res = pthread_rwlock_init(&rwlock, &attr);
    assert(res==0);
    res = pthread_rwlockattr_destroy(&attr);
    assert(res==0);
  }

  SharedMutex::~SharedMutex()
  {
    [[maybe_unused]] int res = pthread_rwlock_destroy(&rwlock);
    assert(res==0);
  }

  void SharedMutex::lock()
  {
    [[maybe_unused]] int res = pthread_rwlock_wrlock(&rwlock);
    assert(res==0);
  }

  void SharedMutex::unlock()
  {
    [[maybe_unused]] int res = pthread_rwlock_unlock(&rwlock);
    assert(res==0);
  }

  void SharedMutex::lock_shared()
  {
    [[maybe_unused]] int res = pthread_rwlock_rdlock(&rwlock);
    assert(res==0);
  }

  void SharedMutex::unlock_shared()
  {
    [[maybe_unused]] int res = pthread_rwlock_unlock(&rwlock);
    assert(res==0);
  }

#endif

}
