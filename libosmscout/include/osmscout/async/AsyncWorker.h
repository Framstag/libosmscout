#ifndef LIBOSMSCOUT_ASYNCWORKER_H
#define LIBOSMSCOUT_ASYNCWORKER_H

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

#include <osmscout/async/Breaker.h>
#include <osmscout/async/CancelableFuture.h>
#include <osmscout/async/WorkQueue.h>

#include <thread>
#include <cassert>

namespace osmscout {

  /** Async worker provides simple tool for providing asynchronous method calls.
   * Functions executed via Async method are executed in contex of worker thread.
   * If all class fields are modified in context of worker thread, there is no
   * need of synchronisation.
   */
  class OSMSCOUT_API AsyncWorker
  {
  private:
    osmscout::ProcessingQueue<std::function<void()>> queue;
    bool deleteOnExit=false;

    // thread have to be initialized after queue
    std::thread thread;

  public:
    explicit AsyncWorker(const std::string &name);
    virtual ~AsyncWorker();

    AsyncWorker(const AsyncWorker&) = delete;
    AsyncWorker(AsyncWorker&&) = delete;

    AsyncWorker& operator=(const AsyncWorker&) = delete;
    AsyncWorker& operator=(AsyncWorker&&) = delete;

    void Loop();

    void DeleteLater();

    std::thread::id GetThreadId() const
    {
      return thread.get_id();
    }

    void ThreadAssert() const
    {
      assert(std::this_thread::get_id()==thread.get_id());
    }

  protected:
    template<typename T>
    CancelableFuture<T> Async(const std::function<T(Breaker&)> &task)
    {
      typename CancelableFuture<T>::Promise promise;
      queue.PushTask([promise, task]() mutable {
        typename CancelableFuture<T>::FutureBreaker breaker=promise.Breaker();
        T result = task(breaker);
        promise.SetValue(result);
      });
      return promise.Future();
    }
  };

}

#endif //LIBOSMSCOUT_ASYNCWORKER_H
