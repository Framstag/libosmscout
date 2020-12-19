#ifndef OSMSCOUT_UTIL_PROCESSINGQUEUE_H
#define OSMSCOUT_UTIL_PROCESSINGQUEUE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020 Tim Teulings

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
#include <memory>
#include <limits>
#include <thread>

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  template<typename R>
  class ProcessingQueue
  {
  private:
    std::mutex              mutex;
    std::condition_variable pushCondition;
    std::condition_variable popCondition;
    std::deque<R>           tasks;
    size_t                  queueLimit;
    bool                    running=true;

  public:
    ProcessingQueue();

    explicit ProcessingQueue(size_t queueLimit);
    ~ProcessingQueue();

    void PushTask(const R& task);
    void PushTask(R&& task);
    std::optional<R> PopTask();

    void Stop();
  };

  template<class R>
  ProcessingQueue<R>::ProcessingQueue()
    : queueLimit(std::numeric_limits<size_t>::max())
  {
    // no code
  }

  template<class R>
  ProcessingQueue<R>::ProcessingQueue(size_t queueLimit)
    : queueLimit(queueLimit)
  {
    // no code
  }

  template<class R>
  ProcessingQueue<R>::~ProcessingQueue() = default;

  template<class R>
  void ProcessingQueue<R>::PushTask(const R& task)
  {
    std::unique_lock<std::mutex> lock(mutex);

    pushCondition.wait(lock,[this]{return tasks.size()<=queueLimit;});

    tasks.push_back(task);

    popCondition.notify_one();
  }

  template<class R>
  void ProcessingQueue<R>::PushTask(R&& task)
  {
    std::unique_lock<std::mutex> lock(mutex);

    pushCondition.wait(lock,[this]{return tasks.size()<=queueLimit;});

    tasks.push_back(std::move(task));

    popCondition.notify_one();
  }

  template<class R>
  std::optional<R> ProcessingQueue<R>::PopTask()
  {
    std::unique_lock<std::mutex> lock(mutex);

    popCondition.wait(lock,[this]{return !tasks.empty() || !running;});

    if (tasks.empty() &&
        !running) {
      return {};
    }

    R task=std::move(tasks.front());
    tasks.pop_front();

    pushCondition.notify_one();

    return task;
  }

  template<class R>
  void ProcessingQueue<R>::Stop()
  {
    std::lock_guard<std::mutex> lock(mutex);

    running=false;

    popCondition.notify_all();
  }
}

#endif
