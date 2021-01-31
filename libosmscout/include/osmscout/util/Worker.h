#ifndef OSMSCOUT_UTIL_WORKER_H
#define OSMSCOUT_UTIL_WORKER_H

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

#include <thread>
#include <optional>

#include <osmscout/util/ProcessingQueue.h>

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  /**
   * A single threaded agent.
   *
   * This is a simple wrapper around the std::thread primitive.
   *
   * Note: ProcessingLoop method is pure virtual in ThreadedWorker.
   * It may be called only after Vtable of child object is constructed.
   * For that reason it is responsibility for child class to call
   * Start method from its constructor. It is also beneficial
   * to mark child classes as final.
   */
  class OSMSCOUT_API ThreadedWorker
  {
  private:
    std::thread thread;
    bool        success=true;

  protected:
    /**
     * Worker can set itself to the failure status. This can later on queried
     * by the parent process after Wait() to stop further processing.
     *
     * The status is not protected by a mutex, please only call from inside the worker.
     *
     * We currently do not support propagation of an detailed error object.
     * We assume that the worker itself logs its error and that the parent worker
     * is only interested in a boolean status.
     *
     * This might change in future.
     */
    void MarkWorkerAsFailed() {
      success=false;
    }

  public:
    ThreadedWorker() = default;

    ThreadedWorker(const ThreadedWorker& other) = delete;
    ThreadedWorker(ThreadedWorker&& other) = delete;

    virtual ~ThreadedWorker() = default;

    /**
     * Returns true if the worker processed successfully, else false.
     *
     * Please do not call before #Wait() return.
     * If called before #Wait() the status may not be final. Also the value is not protected
     * by a mutex.
     *
     * @return true or false
     */
    bool WasSuccessful() const
    {
      return success;
    }

    void Wait() {
      if (thread.joinable()) {
        thread.join();
      }
    }

  protected:
    void Start()
    {
      thread = std::thread(&ThreadedWorker::ProcessingLoop,this);
    }

    virtual void ProcessingLoop() = 0;
  };

  /**
   * A specialisation of Worker. A Producer generates events of type E and
   * places them into an out queue.
   *
   *  If the producer is finished it can be joined. The outQueue is
   *  stopped.
   *
   * @tparam E The event type of the outgoing queue
   */
  template <typename E>
  class Producer : public ThreadedWorker
  {
  protected:
    ProcessingQueue<E>& outQueue;

  public:
    explicit Producer(ProcessingQueue<E>& outQueue)
      : outQueue(outQueue)
    {
    }
  };

  /**
   * A specialisation of worker. A pipe consumes events from a incoming queue and
   * produces new events for a outgoing queue.
   *
   * The incoming queue is processes until it is stopped and empty.
   *
   * @tparam E1 The event type of the incoming queue
   * @tparam E2 The event type of the outgoing queue
   */
  template <typename E1, typename E2>
  class Pipe : public ThreadedWorker
  {
  protected:
    ProcessingQueue<E1>& inQueue;
    ProcessingQueue<E2>& outQueue;

  public:
    Pipe(ProcessingQueue<E1>& inQueue,
         ProcessingQueue<E2>& outQueue)
      : inQueue(inQueue),
        outQueue(outQueue)
    {
    }
  };

  /**
   * A specialisation of worker that consumes events from a queue.
   *
   * The consumer polls the queue and processes the events until the queue
   * is stopped and the remaining events completely consumed.
   *
   * @tparam E the event type of the incoming queue
   */
  template <typename E>
  class Consumer : public ThreadedWorker
  {
  protected:
    ProcessingQueue<E>& inQueue;

  public:
    explicit Consumer(ProcessingQueue<E>& inQueue)
      : inQueue(inQueue)
    {
    }
  };

}

#endif
