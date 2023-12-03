#ifndef LIBOSMSCOUT_THREAD_H
#define LIBOSMSCOUT_THREAD_H

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

#include <string>
#include <thread>
#include <optional>

namespace osmscout {

  /** Try to set current thread name.
   *
   * @param name
   * @return true if supported and successful, else otherwise
   */
  extern OSMSCOUT_API bool SetThreadName(const std::string &name);

  /** Try to set thread name
   *
   * @param thread
   * @param name
   * @return true if supported and successful, else otherwise
   */
  extern OSMSCOUT_API bool SetThreadName(std::thread &thread, const std::string &name);

  /** Signal that is emitted on thread exit.
   * It is not guaranteed that signal will be emitted for all threads,
   * some threads may be still running on program exit. For example detached
   * thread, global scheduler threads, when std::exit is called...
   */
  extern OSMSCOUT_API Signal<std::thread::id>& ThreadExitSignal();
}

#endif //LIBOSMSCOUT_THREAD_H
