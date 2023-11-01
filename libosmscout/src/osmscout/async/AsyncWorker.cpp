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

#include <osmscout/async/AsyncWorker.h>

#include <osmscout/async/Thread.h>

namespace osmscout {

AsyncWorker::AsyncWorker(const std::string &name):
  thread(&AsyncWorker::Loop,this)
{
  Async<int>([name](Breaker &) -> int {
    SetThreadName(name);
    return 0;
  });
}

AsyncWorker::~AsyncWorker()
{
  queue.Stop();
  if (thread.joinable() && thread.get_id() != std::this_thread::get_id()) {
    thread.join();
  } else {
    thread.detach();
  }
}

void AsyncWorker::Loop()
{
  while (true) {
    auto taskOpt = queue.PopTask();
    if (!taskOpt) {
      break;
    }

    taskOpt.value()();
  }

  if (deleteOnExit) {
    delete this;
  }
}

void AsyncWorker::DeleteLater()
{
  Async<bool>([this](Breaker &) -> bool {
    queue.Stop();
    deleteOnExit=true;
    return true;
  });
}
}
