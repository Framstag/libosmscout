/*
  AsyncProcessing - a test program for libosmscout
  Copyright (C) 2020  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include <osmscout/async/ProcessingQueue.h>
#include <osmscout/util/StopClock.h>

#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

static size_t iterationCount=4000;
static auto   taskDuration=1ms;

class IntWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

public:
  size_t                          processedCount;

private:
  void ProcessorLoop()
  {
    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        break;
      }

      if (value<0) {
        break;
      }

      std::this_thread::sleep_for(taskDuration);

      processedCount++;
    }
  }

public:
  explicit IntWorker(osmscout::ProcessingQueue<int>& queue)
  : queue(queue),
    thread(&IntWorker::ProcessorLoop,this),
    processedCount(0)
  {

  }

  void Wait() {
    thread.join();
  }
};

struct VectorData
{
  int id;
  std::vector<int> data;

  explicit VectorData(int id)
  : id(id)
  {
  }

  VectorData(const VectorData& other) = delete;
  VectorData(VectorData&& other) = default;
};

class VectorWorker
{
public:
  size_t processedCount;

private:
  osmscout::ProcessingQueue<VectorData>& queue;
  std::thread                             thread;

private:
  void ProcessorLoop()
  {
    while (true) {
      std::optional<VectorData> value=queue.PopTask();

      if (!value) {
        break;
      }

      if (value.value().data.empty()) {
        break;
      }

      std::this_thread::sleep_for(taskDuration);

      processedCount++;
    }
  }

public:
  explicit VectorWorker(osmscout::ProcessingQueue<VectorData>& queue)
  : processedCount(0),
    queue(queue),
    thread(&VectorWorker::ProcessorLoop,this)
  {

  }

  void Wait() {
    thread.join();
  }
};

TEST_CASE("AsyncProcessing")
{
  {
    osmscout::ProcessingQueue<int> queue(1000);
    IntWorker                      worker(queue);

    for (size_t i=1; i<=iterationCount; i++) {
      queue.PushTask(i);
    }

    queue.PushTask(-1);
    queue.Stop();

    worker.Wait();

    REQUIRE(worker.processedCount==iterationCount);
  }

  {
    osmscout::ProcessingQueue<VectorData> queue(1000);
    VectorWorker                           worker(queue);

    for (size_t i=1; i<=iterationCount; i++) {
      VectorData data(i);
      data.data.assign(i,i);
      queue.PushTask(std::move(data));
    }

    queue.PushTask(VectorData(-1));
    queue.Stop();

    worker.Wait();

    REQUIRE(worker.processedCount==iterationCount);
  }

  {
    osmscout::ProcessingQueue<VectorData> queue(1000);
    VectorWorker                           worker1(queue);
    VectorWorker                           worker2(queue);
    VectorWorker                           worker3(queue);
    VectorWorker                           worker4(queue);

    for (size_t i=1; i<=iterationCount; i++) {
      VectorData data(i);
      data.data.assign(i,i);
      queue.PushTask(std::move(data));
    }

    queue.PushTask(VectorData(-1));
    queue.Stop();

    worker1.Wait();
    worker2.Wait();
    worker3.Wait();
    worker4.Wait();

    REQUIRE(worker1.processedCount+worker2.processedCount+worker3.processedCount+
            worker4.processedCount==iterationCount);
  }
}