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

#include <osmscout/util/ProcessingQueue.h>

#include <osmscout/CoreImportExport.h>

namespace osmscout {

  class OSMSCOUT_API ThreadedWorker
  {
  private:
    std::thread thread;

  public:
    ThreadedWorker()
    : thread(&ThreadedWorker::ProcessingLoop,this)
    {
    }

    ThreadedWorker(const ThreadedWorker& other) = delete;
    ThreadedWorker(ThreadedWorker&& other) = delete;

    virtual ~ThreadedWorker() = default;

    void Wait() {
      thread.join();
    }

  protected:
    virtual void ProcessingLoop() = 0;
  };

  template <typename E>
  class OSMSCOUT_API Producer : public ThreadedWorker
  {
  protected:
    ProcessingQueue<E>& outQueue;

  public:
    Producer(ProcessingQueue<E>& outQueue)
      : outQueue(outQueue)
    {
    }
  };

  template <typename E1, typename E2>
  class OSMSCOUT_API Pipe : public ThreadedWorker
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

  template <typename E>
  class OSMSCOUT_API Consumer : public ThreadedWorker
  {
  protected:
    ProcessingQueue<E>& inQueue;

  public:
    Consumer(ProcessingQueue<E>& inQueue)
      : inQueue(inQueue)
    {
    }
  };

}

#endif
