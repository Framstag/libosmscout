#ifndef LIBOSMSCOUT_THREADFINALIZER_H
#define LIBOSMSCOUT_THREADFINALIZER_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2023 Lukas Karas

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

#include <osmscout/async/Signal.h>

#include <thread>

namespace osmscout {

  struct OSMSCOUT_API ThreadFinalizer
  {
    osmscout::Signal<std::thread::id> threadExit;

    ThreadFinalizer() = default;
    ThreadFinalizer(const ThreadFinalizer &) = delete;
    ThreadFinalizer(ThreadFinalizer &&) = delete;

    virtual ~ThreadFinalizer()
    {
      threadExit.Emit(std::this_thread::get_id());
    }

    ThreadFinalizer &operator=(const ThreadFinalizer &) = delete;
    ThreadFinalizer &operator=(ThreadFinalizer &&) = delete;
  };

  /** Thread local thread finalizer that provides signal on thread exit.
   * It is not guaranteed that signal will be emitted for all threads,
   * some threads may be still running on program exit. For example detached
   * thread, global scheduler threads, when std::exit is called...
   */
  extern OSMSCOUT_API ThreadFinalizer& ThreadFinalizer();
}

#endif //LIBOSMSCOUT_THREADFINALIZER_H
