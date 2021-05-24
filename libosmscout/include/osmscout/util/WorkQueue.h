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

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  template<typename R>
  class WorkQueue
  {
  private:
    using Task = std::packaged_task<R ()>;

  private:
    std::mutex              mutex;
    std::condition_variable pushCondition;
    std::condition_variable popCondition;
    std::deque<Task>        tasks;
    size_t                  queueLimit=std::numeric_limits<size_t>::max();
    bool                    running=true;

  public:
    WorkQueue();

    explicit WorkQueue(size_t queueLimit);
    ~WorkQueue();

    void PushTask(Task& task);
    bool PopTask(Task& task);

    void Stop();
  };

  template<class R>
  WorkQueue<R>::WorkQueue() = default;

  template<class R>
  WorkQueue<R>::WorkQueue(size_t queueLimit)
    : queueLimit(queueLimit)
  {
    // no code
  }

  template<class R>
  WorkQueue<R>::~WorkQueue() = default;

  template<class R>
  void WorkQueue<R>::PushTask(Task& task)
  {
    std::unique_lock lock(mutex);

    pushCondition.wait(lock,[this]{return tasks.size()<=queueLimit;});

    tasks.push_back(std::move(task));

    popCondition.notify_one();
  }

  template<class R>
  bool WorkQueue<R>::PopTask(Task& task)
  {
    std::unique_lock lock(mutex);

    popCondition.wait(lock,[this]{return !tasks.empty() || !running;});

    if (tasks.empty() &&
        !running) {
      return false;
    }

    task=std::move(tasks.front());
    tasks.pop_front();

    pushCondition.notify_one();

    return true;
  }

  template<class R>
  void WorkQueue<R>::Stop()
  {
    std::scoped_lock<std::mutex> lock(mutex);

    running=false;

    popCondition.notify_all();
  }
}

#endif
