#ifndef OSMSCOUT_UTIL_WORKQUEUE_H
#define OSMSCOUT_UTIL_WORKQUEUE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2015 Tim Teulings

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

#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <limits>
#include <thread>

#include <osmscout/lib/CoreImportExport.h>
#include <osmscout/async/ProcessingQueue.h>

namespace osmscout {

  template<typename R>
  class WorkQueue: public ProcessingQueue<std::packaged_task<R ()>>
  {
  private:
    using Task = std::packaged_task<R ()>;

  public:
    WorkQueue() = default;

    explicit WorkQueue(size_t queueLimit);
    ~WorkQueue() override = default;

    bool PopTask(Task& task);
  };


  template<class R>
  WorkQueue<R>::WorkQueue(size_t queueLimit)
    : ProcessingQueue<Task>(queueLimit)
  {
    // no code
  }

  template<class R>
  bool WorkQueue<R>::PopTask(Task& task)
  {
    auto taskOpt = ProcessingQueue<Task>::PopTask();

    if (!taskOpt) {
      return false;
    }

    task=std::move(taskOpt.value());

    return true;
  }
}

#endif
