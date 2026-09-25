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

    /**
     * Stops the worker: the queue stops accepting jobs and the job that is currently running is
     * waited for. Harmless when called more than once.
     *
     * A derived class whose jobs read or write its members must call this at the beginning of its
     * own destructor: the base class is destroyed after the derived members, so the worker thread
     * would otherwise keep running a job against state that is already gone. When the call comes
     * from the worker thread itself there is nothing to wait for, so the thread is detached.
     */
    void Stop();

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
        T result{};
        try {
          result=task(breaker);
        } catch (const std::exception &e) {
          // A job must not be able to take the process down. The failure is reported here (and
          // again by the worker loop), and the promise is resolved with the default value of the
          // result type so that a caller waiting for the job is released instead of waiting
          // forever - which means such a caller cannot treat the value as a result.
          log.Error() << "Async job failed: " << e.what();
        } catch (...) {
          log.Error() << "Async job failed with an unknown exception";
        }
        promise.SetValue(result);
      });
      return promise.Future();
    }
  };

}

#endif //LIBOSMSCOUT_ASYNCWORKER_H
