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
#include <osmscout/log/Logger.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace osmscout {

AsyncWorker::AsyncWorker(const std::string &name):
  name(name),
  thread(&AsyncWorker::Loop,this)
{
  Async<int>([name](Breaker &) -> int {
    SetThreadName(name);
    return 0;
  });
}

AsyncWorker::~AsyncWorker()
{
  if (!shutdownRequested && HasInFlightJobs()) {
    log.Error() << "AsyncWorker '" << name
                << "' is destroyed without stopping its worker: a job that is still running"
                   " may access its already destroyed state";
  }

  Stop();

  // Last chance to dispose of the worker thread, for the case that a derived destructor had to stop
  // the worker from the worker thread itself.
  if (thread.joinable()) {
    if (thread.get_id() != std::this_thread::get_id()) {
      thread.join();
    } else {
      thread.detach();
    }
  }
}

void AsyncWorker::Stop()
{
  std::vector<BreakerRef> inFlight;

  {
    std::unique_lock<std::mutex> lock(breakerMutex);

    if (shutdownRequested) {
      return;
    }
    shutdownRequested=true;

    inFlight.swap(breakers);
  }

  // Stop the queue and drop the jobs that have not started in one step, so that the job that is
  // returning right now cannot pick one of them up. A job that is submitted after this point is not
  // queued at all (see RegisterBreaker).
  queue.StopAndDiscard();

  // Ask the jobs that are running to stop, which bounds the wait below. A job that does not poll its
  // breaker keeps running and is waited for.
  for (const auto &breaker:inFlight) {
    breaker->Break();
  }

  // Wait for the job that is running, unless this runs on the worker thread itself, where waiting is
  // impossible. The destructor disposes of a thread that was not joined.
  if (thread.joinable() && thread.get_id() != std::this_thread::get_id()) {
    thread.join();
  }
}

bool AsyncWorker::RegisterBreaker(const BreakerRef &breaker)
{
  std::unique_lock<std::mutex> lock(breakerMutex);

  if (shutdownRequested) {
    return false;
  }

  breakers.push_back(breaker);

  return true;
}

void AsyncWorker::UnregisterBreaker(const BreakerRef &breaker)
{
  std::unique_lock<std::mutex> lock(breakerMutex);

  breakers.erase(std::ranges::remove(breakers, breaker).begin(),
                 breakers.end());
}

bool AsyncWorker::HasInFlightJobs()
{
  std::unique_lock<std::mutex> lock(breakerMutex);

  return !breakers.empty();
}

void AsyncWorker::Loop()
{
  while (!queue.Finished()) {
    auto taskOpt = queue.PopTask();
    if (!taskOpt) {
      continue;
    }

    // Backstop for jobs that are not submitted through Async<T>: a job must never be able to
    // escape this thread, because an escaping exception terminates the process (and with it a
    // host JVM) instead of failing a single job.
    try {
      taskOpt.value()();
    } catch (const std::exception &e) {
      log.Error() << "Async job failed: " << e.what();
    } catch (...) {
      log.Error() << "Async job failed with an unknown exception";
    }
  }

  if (deleteOnExit) {
    delete this;
  }
}

void AsyncWorker::DeleteLater()
{
  Async<bool>([this](Breaker &) -> bool {
    // Called on the worker thread: stopping here must not wait for this thread, so the job is not
    // joined here and the worker ends after the queue has been drained. The thread is disposed of by
    // the destructor.
    Stop();
    deleteOnExit=true;
    return true;
  });
}
}
