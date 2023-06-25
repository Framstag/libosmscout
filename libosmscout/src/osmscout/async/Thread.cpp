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

#include <osmscout/private/Config.h>
#include <osmscout/async/Thread.h>

#ifdef OSMSCOUT_PTHREAD_NAME
#include <type_traits>

#include <pthread.h>

static_assert(std::is_same<std::thread::native_handle_type, pthread_t>::value, "std::thread::native_handle_type have to be pthread_t");
#endif

namespace osmscout {

bool SetThreadName([[maybe_unused]] const std::string &name)
{
#ifdef OSMSCOUT_PTHREAD_NAME
  return pthread_setname_np(pthread_self(), name.c_str()) == 0;
#else
  return false;
#endif
}

#ifdef OSMSCOUT_PTHREAD_NAME
bool SetThreadName(std::thread &thread, const std::string &name)
{
    return pthread_setname_np(thread.native_handle(), name.c_str()) == 0;
}
#else
bool SetThreadName(std::thread &/*thread*/, const std::string &/*name*/)
{
    return false;
}
#endif
}
