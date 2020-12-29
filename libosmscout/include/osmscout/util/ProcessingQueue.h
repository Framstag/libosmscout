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
#include <limits>
#include <memory>
#include <mutex>
#include <optional>

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  template<typename T>
  class ProcessingQueue
  {
  private:
    std::mutex              mutex;
    std::condition_variable pushCondition;
    std::condition_variable popCondition;
    std::deque<T>           tasks;
    size_t                  queueLimit;
    bool                    running{true};

  public:
    ProcessingQueue();
    explicit ProcessingQueue(size_t queueLimit);

    ProcessingQueue(const ProcessingQueue&) = delete;
    ProcessingQueue &operator=(const ProcessingQueue&) = delete;

    ~ProcessingQueue() = default;

    void PushTask(const T& task);
    void PushTask(T&& task);
    std::optional<T> PopTask();

    void Stop();
  };

  template<class T>
  ProcessingQueue<T>::ProcessingQueue()
    : queueLimit(std::numeric_limits<size_t>::max())
  {
    // no code
  }

  template<class T>
  ProcessingQueue<T>::ProcessingQueue(size_t queueLimit)
    : queueLimit(queueLimit)
  {
    // no code
  }

  template<class T>
  void ProcessingQueue<T>::PushTask(const T& task)
  {
    std::unique_lock lock(mutex);

    pushCondition.wait(lock,[this]{return tasks.size()<queueLimit;});

    tasks.push_back(task);

    lock.unlock();

    popCondition.notify_one();
  }

  template<class T>
  void ProcessingQueue<T>::PushTask(T&& task)
  {
    std::unique_lock lock(mutex);

    pushCondition.wait(lock,[this]{return tasks.size()<queueLimit;});

    tasks.push_back(std::forward<T>(task));

    lock.unlock();

    popCondition.notify_one();
  }

  template<class T>
  std::optional<T> ProcessingQueue<T>::PopTask()
  {
    std::unique_lock lock(mutex);

    popCondition.wait(lock,[this]{return !tasks.empty() || !running;});

    if (!running &&
        tasks.empty()) {
      return {};
    }

    T task=std::move(tasks.front());
    tasks.pop_front();

    lock.unlock();

    pushCondition.notify_one();

    return task;
  }

  template<class R>
  void ProcessingQueue<R>::Stop()
  {
    std::unique_lock lock(mutex);

    running=false;

    lock.unlock();

    popCondition.notify_all();
  }
}

#endif
