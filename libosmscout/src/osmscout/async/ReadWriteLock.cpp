/*
 This source is part of the libosmscout library
 Copyright (C) 2024  Jean-Luc Barriere

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

#include <osmscout/log/Logger.h>

#include <cassert>
#include <chrono>

namespace osmscout {

/**
 * The X flag is set as follows based on the locking steps
 * Step 0 : X is released
 * Step 1 : X is held, but waits for release of S
 * Step 2 : X was released and left available for one of request in wait
 * Step 3 : X is held
 * Step N : X recursive N-3
 */
constexpr int X_STEP_0 = 0;
constexpr int X_STEP_1 = 1;
constexpr int X_STEP_2 = 2;
constexpr int X_STEP_3 = 3;

void Latch::lock() {
  /* Depending on the internal implementation of conditional variable,
   * a race condition could arise, permanently blocking the thread;
   * Setting a timeout works around the issue.
   */
  static constexpr std::chrono::seconds exit_timeout(1);

  std::thread::id tid = std::this_thread::get_id();

  spin_lock();

  if (x_owner != tid) {
    /* increments the count of request in wait */
    ++x_wait;
    for (;;) {
      /* if flag is 0 or 2 then it hold X with no wait,
       * in other case it have to wait for X gate
       */
      if (x_flag == X_STEP_0 || x_flag == X_STEP_2) {
        x_flag = X_STEP_1;
        --x_wait;
        break;
      } else {
        /* !!! pop gate then unlock spin */
        std::unique_lock<std::mutex> lk(x_gate_lock);
        spin_unlock();
        x_gate.wait_for(lk, exit_timeout);
        lk.unlock();
      }
      spin_lock();
    }

    /* X = 1, check the releasing of S */
    for (;;) {
      /* if the count of S is zeroed then it finalize with no wait,
       * in other case it have to wait for S gate */
      if (s_count == 0) {
        x_flag = X_STEP_3;
        break;
      } else {
        /* !!! pop gate then unlock spin (reverse order for S notifier) */
        std::unique_lock<std::mutex> lk(s_gate_lock);
        spin_unlock();
        s_gate.wait_for(lk, exit_timeout);
        lk.unlock();
        spin_lock();
        /* check if the notifier has hand over, else retry */
        if (x_flag == X_STEP_3) {
          break;
        }
      }
    }

    /* X = 3, set owner */
    x_owner = tid;
  } else {
    /* recursive X lock */
    ++x_flag;
  }

  spin_unlock();
}

void Latch::unlock() {
  spin_lock();
  if (x_owner == std::this_thread::get_id()) {
    /* decrement recursive lock */
    if (--x_flag == X_STEP_2) {
      x_owner = std::thread::id();
      /* hand-over to a request in wait for X, else release */
      if (x_wait == 0) {
        x_flag = X_STEP_0;
      }
      /* !!! unlock spin then pop gate (reverse order for receiver) */
      spin_unlock();
      std::unique_lock<std::mutex> lk(x_gate_lock);
      x_gate.notify_all();
      lk.unlock();
    } else {
      spin_unlock();
    }
  } else {
    spin_unlock();
  }
}

namespace {
static const std::hash<std::thread::id> thread_hash;
}

void Latch::lock_shared() {
  /* Depending on the internal implementation of conditional variable,
   * a race condition could arise, permanently blocking the thread;
   * Setting a timeout works around the issue.
   */
  static constexpr std::chrono::seconds exit_timeout(1);

  std::thread::id tid = std::this_thread::get_id();

  spin_lock();
  if (x_owner != tid) {
    /* if flag is 0 or 1 then it hold S with no wait,
     * in other case it have to wait for X gate
     */
    for (;;) {
      if (!px) {
        /* X precedence is false */
        if (x_flag < X_STEP_2) {
          break;
        }
      } else {
        /* X precedence is true,
         * estimate if this thread holds a recursive S lock
         */
        if (x_flag == X_STEP_0 || (x_flag == X_STEP_1 &&
                                   s_buckets[thread_hash(tid) % latch_bucket_count] > 0)) {
          break;
        }
      }
      /* !!! pop gate then unlock spin */
      std::unique_lock<std::mutex> lk(x_gate_lock);
      spin_unlock();
      x_gate.wait_for(lk, exit_timeout);
      lk.unlock();
      spin_lock();
    }
  }
  ++s_count;
  if (px) {
    /* X precedence is true */
    ++s_buckets[thread_hash(tid) % latch_bucket_count];
  }
  spin_unlock();
}

void Latch::unlock_shared() {
  std::thread::id tid = std::this_thread::get_id();

  spin_lock();
  if (px) {
    /* X precedence is true */
    --s_buckets[thread_hash(tid) % latch_bucket_count];
  }
  /* on last S, finalize X request in wait, and notify */
  if (--s_count == 0 && x_flag == X_STEP_1 && x_owner != tid) {
    x_flag = X_STEP_3;
    /* !!! unlock spin then pop gate (reverse order for X receiver) */
    spin_unlock();
    std::unique_lock<std::mutex> lk(s_gate_lock);
    s_gate.notify_one();
    lk.unlock();
  } else {
    spin_unlock();
  }
}

bool Latch::try_lock_shared()
{
  std::thread::id tid = std::this_thread::get_id();

  spin_lock();
  /* if X = 0 then it hold S with success,
   * in other case fails
   */
  if (x_flag == X_STEP_0 || x_owner == tid) {
    ++s_count;
    if (px) {
      /* X precedence is true */
      ++s_buckets[thread_hash(tid) % latch_bucket_count];
    }
    spin_unlock();
    return true;
  }
  spin_unlock();
  return false;
}

}
