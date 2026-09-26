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
#include <osmscout/log/Logger.h>

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cassert>

namespace osmscout {

  /** Async worker provides simple tool for providing asynchronous method calls.
   * Functions executed via Async method are executed in contex of worker thread.
   * If all class fields are modified in context of worker thread, there is no
   * need of synchronisation.
   *
   * Ownership rule: a job runs against the object it was submitted on, so the
   * object must stop its worker before its own state is destroyed. A class that
   * derives from AsyncWorker and runs jobs that touch its members therefore
   * calls Stop() at the beginning of its destructor, because its members are
   * destroyed before the base class destructor runs:
   *
   * \code
   * ~MyWorker() override
   * {
   *   Stop();
   * }
   * \endcode
   */
  class OSMSCOUT_API AsyncWorker
  {
  private:
    std::string             name;
    std::atomic<bool>       shutdownRequested{false};
    std::mutex              breakerMutex;
    std::vector<BreakerRef> breakers;

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
     * Stops the worker: it breaks the breakers of the jobs that are in flight, stops the queue,
     * discards the jobs that have not started, and then waits for the job that is running, so that no
     * job runs against state that is gone. It is harmless to call this more than once.
     *
     * A derived class whose jobs read or write its members must call this at the beginning of its own
     * destructor: the base class is destroyed after the derived members, so the worker thread would
     * otherwise keep running a job against state that is already gone.
     *
     * A job that polls the Breaker it received stops at its next check, which bounds the wait; a job that
     * does not poll is still waited for, so correctness does not depend on a job cooperating. When the call
     * comes from the worker thread itself there is nothing to wait for, and the destructor disposes of the
     * thread - which also keeps GetThreadId() usable for a derived destructor.
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
      BreakerRef breaker=std::make_shared<typename CancelableFuture<T>::FutureBreaker>(promise.Breaker());

      if (!RegisterBreaker(breaker)) {
        // The worker already stopped: the job is not queued, so cancel the future instead of leaving
        // a caller that waits for it without an answer.
        breaker->Break();
        promise.Cancel();
        return promise.Future();
      }

      queue.PushTask([this, breaker, promise, task]() mutable {
        T result{};
        try {
          result=task(*breaker);
        } catch (const std::exception &e) {
          // A job must not be able to take the process down. The failure is reported here (and again
          // by the worker loop), and the promise is resolved with the default value of the result type
          // so that a caller waiting for the job is released instead of waiting forever - which means
          // such a caller cannot treat the value as a result.
          log.Error() << "Async job failed: " << e.what();
        } catch (...) {
          log.Error() << "Async job failed with an unknown exception";
        }
        UnregisterBreaker(breaker);
        promise.SetValue(result);
      });

      return promise.Future();
    }

  private:
    /**
     * Register a job's breaker while the job is in flight. Returns false when the worker already
     * stopped, in which case the job must not run.
     */
    bool RegisterBreaker(const BreakerRef &breaker);

    void UnregisterBreaker(const BreakerRef &breaker);

    /** True when a job has been submitted and has not finished yet. */
    bool HasInFlightJobs();
  };

}

#endif //LIBOSMSCOUT_ASYNCWORKER_H
