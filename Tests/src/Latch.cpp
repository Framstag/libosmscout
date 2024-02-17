/*
  Latch - a test program for libosmscout
  Copyright (C) 2024  Jean-Luc Barriere

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  asize_t with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include <osmscout/async/ReadWriteLock.h>
#include <osmscout/async/ProcessingQueue.h>
#include <osmscout/util/StopClock.h>

#include <TestMain.h>

using namespace std::chrono_literals;

static size_t iterationCount=250;
static auto   taskDuration=1ms;

static size_t refCounter = 0;
static osmscout::Latch latch;

class ReaderWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

public:
  size_t                          processedCount;

private:
  void ProcessorLoop()
  {
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      if (value<0) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      {
        osmscout::ReadLock locker(latch);
        [[maybe_unused]] size_t c = refCounter;
        std::this_thread::sleep_for(taskDuration);
      }

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit ReaderWorker(osmscout::ProcessingQueue<int>& queue)
  : queue(queue),
    thread(&ReaderWorker::ProcessorLoop,this),
    processedCount(0)
  {
  }

  void Wait() {
    thread.join();
  }
};

class WriterWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

public:
  size_t                          processedCount;

private:
  void ProcessorLoop()
  {
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      if (value<0) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      {
        osmscout::WriteLock locker(latch);
        ++refCounter;
        std::this_thread::sleep_for(taskDuration);
      }

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit WriterWorker(osmscout::ProcessingQueue<int>& queue)
      : queue(queue),
      thread(&WriterWorker::ProcessorLoop,this),
      processedCount(0)
  {
  }

  void Wait() {
    thread.join();
  }
};

class ReaderReaderWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

public:
  size_t                          processedCount;

private:
  void ProcessorLoop()
  {
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      if (value<0) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      {
        osmscout::ReadLock rl1(latch);
        {
          osmscout::ReadLock rl2(latch);
          {
            osmscout::ReadLock rl3(latch);
            [[maybe_unused]] size_t c = refCounter;
            std::this_thread::sleep_for(taskDuration);
          }
        }
      }

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit ReaderReaderWorker(osmscout::ProcessingQueue<int>& queue)
      : queue(queue),
      thread(&ReaderReaderWorker::ProcessorLoop,this),
      processedCount(0)
  {
  }

  void Wait() {
    thread.join();
  }
};

class WriterReaderWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

public:
  size_t                          processedCount;

private:
  void ProcessorLoop()
  {
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      if (value<0) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      {
        osmscout::WriteLock wr1(latch);
        {
          osmscout::WriteLock wr2(latch);
          {
            osmscout::ReadLock rl1(latch);
            {
              osmscout::ReadLock rl2(latch);
              [[maybe_unused]] size_t c = refCounter;
            }
          }
          ++refCounter;
        }
        std::this_thread::sleep_for(taskDuration);
      }

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit WriterReaderWorker(osmscout::ProcessingQueue<int>& queue)
      : queue(queue),
      thread(&WriterReaderWorker::ProcessorLoop,this),
      processedCount(0)
  {
  }

  void Wait() {
    thread.join();
  }
};

TEST_CASE("Multi Reader Worker") {
  osmscout::StopClock            stopClock;
  osmscout::ProcessingQueue<int> queue(1000);
  ReaderWorker                   worker1(queue);
  ReaderWorker                   worker2(queue);
  ReaderWorker                   worker3(queue);
  ReaderWorker                   worker4(queue);

  std::cout << "Pushing tasks..." << std::endl;

  for (size_t i=1; i<=iterationCount; i++) {
    queue.PushTask(i);
  }

  queue.PushTask(-1);

  queue.Stop();

  std::cout << "Pushing tasks...done, waiting..." << std::endl;

  worker1.Wait();
  worker2.Wait();
  worker3.Wait();
  worker4.Wait();

  stopClock.Stop();

  size_t pc = worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount;

  std::cout << "#processed: "
            << pc << " ["
            << worker1.processedCount << ","
            << worker2.processedCount << ","
            << worker3.processedCount << ","
            << worker4.processedCount << "]"
            << std::endl;
  std::cout << "<<< MultiReaderWorker...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;

  REQUIRE(pc == iterationCount);
}

TEST_CASE("Multi Writer Worker") {
  osmscout::StopClock                   stopClock;
  osmscout::ProcessingQueue<int>        queue(1000);
  WriterWorker                          worker1(queue);
  WriterWorker                          worker2(queue);
  WriterWorker                          worker3(queue);
  WriterWorker                          worker4(queue);

  std::cout << "Pushing tasks..." << std::endl;

  for (size_t i=1; i<=iterationCount; i++) {
    queue.PushTask(i);
  }

  queue.PushTask(-1);

  queue.Stop();

  worker1.Wait();
  worker2.Wait();
  worker3.Wait();
  worker4.Wait();

  std::cout << "Pushing tasks...done, waiting..." << std::endl;


  stopClock.Stop();

  size_t pc = worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount;

  std::cout << "#processed: (" << refCounter << ") "
            << pc << " ["
            << worker1.processedCount << ","
            << worker2.processedCount << ","
            << worker3.processedCount << ","
            << worker4.processedCount << "]"
            << std::endl;
  std::cout << "<<< MultiWriterWorker...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;

  REQUIRE(pc == iterationCount);
  REQUIRE(refCounter == pc);
}

TEST_CASE("Multi Reader Worker One Writer worker") {
  osmscout::StopClock                   stopClock;
  osmscout::ProcessingQueue<int>        queue(1000);
  WriterWorker                          worker1(queue);
  ReaderWorker                          worker2(queue);
  ReaderWorker                          worker3(queue);
  ReaderWorker                          worker4(queue);

  refCounter = 0; // reset counter

  std::cout << "Pushing tasks..." << std::endl;

  for (size_t i=1; i<=iterationCount; i++) {
    queue.PushTask(i);
  }

  queue.PushTask(-1);

  queue.Stop();

  worker1.Wait();
  worker2.Wait();
  worker3.Wait();
  worker4.Wait();

  std::cout << "Pushing tasks...done, waiting..." << std::endl;


  stopClock.Stop();

  size_t pc = worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount;

  std::cout << "#processed: (" << refCounter << ") "
            << pc << " ["
            << worker1.processedCount << ","
            << worker2.processedCount << ","
            << worker3.processedCount << ","
            << worker4.processedCount << "]"
            << std::endl;
  std::cout << "<<< MultiReaderOneWriterWorker...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;

  REQUIRE(pc == iterationCount);
}

TEST_CASE("Multi Recursive Reader Worker") {
  osmscout::StopClock                   stopClock;
  osmscout::ProcessingQueue<int>        queue(1000);
  ReaderReaderWorker                    worker1(queue);
  ReaderReaderWorker                    worker2(queue);
  ReaderReaderWorker                    worker3(queue);
  ReaderReaderWorker                    worker4(queue);

  std::cout << "Pushing tasks..." << std::endl;

  for (size_t i=1; i<=iterationCount; i++) {
    queue.PushTask(i);
  }

  queue.PushTask(-1);

  queue.Stop();

  worker1.Wait();
  worker2.Wait();
  worker3.Wait();
  worker4.Wait();

  std::cout << "Pushing tasks...done, waiting..." << std::endl;


  stopClock.Stop();

  size_t pc = worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount;

  std::cout << "#processed: "
            << pc << " ["
            << worker1.processedCount << ","
            << worker2.processedCount << ","
            << worker3.processedCount << ","
            << worker4.processedCount << "]"
            << std::endl;
  std::cout << "<<< MultiReentrantReaderWorker...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;

  REQUIRE(pc == iterationCount);
}

TEST_CASE("Multi Recursive Writer Worker") {
  osmscout::StopClock                   stopClock;
  osmscout::ProcessingQueue<int>        queue(1000);
  WriterReaderWorker                    worker1(queue);
  WriterReaderWorker                    worker2(queue);
  WriterReaderWorker                    worker3(queue);
  WriterReaderWorker                    worker4(queue);

  refCounter = 0; // reset counter

  std::cout << "Pushing tasks..." << std::endl;

  for (size_t i=1; i<=iterationCount; i++) {
    queue.PushTask(i);
  }

  queue.PushTask(-1);

  queue.Stop();

  worker1.Wait();
  worker2.Wait();
  worker3.Wait();
  worker4.Wait();

  std::cout << "Pushing tasks...done, waiting..." << std::endl;


  stopClock.Stop();

  size_t pc = worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount;

  std::cout << "#processed: (" << refCounter << ") "
            << pc << " ["
            << worker1.processedCount << ","
            << worker2.processedCount << ","
            << worker3.processedCount << ","
            << worker4.processedCount << "]"
            << std::endl;
  std::cout << "<<< MultiReentrantWriterWorker...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;

  REQUIRE(pc == iterationCount);
  REQUIRE(refCounter == pc);
}

TEST_CASE("Check Re-Entrant One Reader Writer") {
  osmscout::StopClock stopClock;
  volatile int i=0;
  osmscout::ReadLock rl(latch);

  std::thread t([&i](){
    osmscout::ReadLock rl0(latch);
    {
      osmscout::WriteLock wl(latch);
      {
        osmscout::ReadLock rl1(latch);
        i++;
      }
    }
  });

  // wait until writer lock is requested
  while (true) {
    if (!latch.try_lock_shared()) {
      // writer lock is requested already
      break;
    }
    latch.unlock_shared();
    std::this_thread::yield();
  }

  REQUIRE(i == 0);
  rl.unlock();
  t.join();

  stopClock.Stop();

  REQUIRE(i == 1);
  std::cout << "<<< ReentrantOneReaderWriter...done: " << stopClock.ResultString() << std::endl;
  std::cout << std::endl;
}

TEST_CASE("Check write precedence") {
  volatile int i=0;
  osmscout::ReadLock rl(latch);

  std::thread t([&i](){
    osmscout::WriteLock wl(latch);
    i++;
  });

  // wait until writer lock is requested
  while (true) {
    if (!latch.try_lock_shared()) {
      // writer lock is requested already
      break;
    }
    latch.unlock_shared();
    std::this_thread::yield();
  }

  REQUIRE(i == 0);
  rl.unlock();

  osmscout::ReadLock rl2(latch); // we should not get shared lock until writer is done
  REQUIRE(i == 1);
  t.join();
}

TEST_CASE("Second shared lock should be blocked when exclusive is requested") {
  int nbreader=4;
  volatile int i=0;
  int blocked=0;
  std::atomic<int> beg=0;
  std::atomic<int> end=0;
  std::vector<std::thread*> pools;
  {
    osmscout::ReadLock rl(latch);

    std::thread t([&i](){
      osmscout::WriteLock wl(latch);
      i++;
    });

    // wait until writer lock is requested
    while (true) {
      if (!latch.try_lock_shared()) {
        // writer lock is requested already
        break;
      }
      latch.unlock_shared();
      std::this_thread::yield();
    }

    for (int nr=0; nr < nbreader; ++nr) {
      std::thread * tr = new std::thread([&beg, &end](){
        beg++;
        osmscout::ReadLock rl(latch);
        end++;
      });
      pools.push_back(tr);
    }

    // wait for everyone to get set up
    int k=0;
    while (beg.load() != nbreader && k++ < 1000) {
      std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(i == 0); // write lock is still waiting
    blocked = beg.load() - end.load();
    rl.unlock();
    t.join();
  }
  std::cout << "#blocked: " << blocked << "/" << nbreader << std::endl;
  // hoping that all read locks have been blocked because exclusive lock was requested
  REQUIRE((blocked == nbreader));
  // check BUG: thread was not awakened after broadcast signal
  // wait for all readers, or fail when lost reader
  int k=0;
  while (end.load() != nbreader && k++ < 1000) {
    std::this_thread::yield();
  }
  // all readers must be finalized
  REQUIRE(end.load() == nbreader);
  // cleanup
  while (!pools.empty()) {
    pools.back()->join();
    delete pools.back();
    pools.pop_back();
  }
}

