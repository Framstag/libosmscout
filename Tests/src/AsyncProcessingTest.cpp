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
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<int> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      //std::cout << "#" << std::this_thread::get_id() << " Value: " << value.value() << std::endl;

      if (value<0) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      std::this_thread::sleep_for(taskDuration);

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
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
  int              id;
  std::vector<int> data;

  explicit VectorData(int id)
  : id(id)
  {
    //std::cout << "Default constructed vector " << id << std::endl;
  }

  VectorData(const VectorData& other) = delete;
  VectorData(VectorData&& other) = default;

};

class VectorWorker
{
private:
  osmscout::ProcessingQueue<VectorData>& queue;
  std::thread                            thread;

public:
  size_t                                 processedCount;

private:
  void ProcessorLoop()
  {
    std::cout << "Processing..." << std::endl;

    while (true) {
      std::optional<VectorData> value=queue.PopTask();

      if (!value) {
        std::cout << "Queue empty!" << std::endl;
        break;
      }

      //std::cout << "#" << std::this_thread::get_id() << " Value: " << value.value().id << std::endl;

      if (value.value().data.empty()) {
        std::cout << "Stop signal fetched!" << std::endl;
        break;
      }

      std::this_thread::sleep_for(taskDuration);

      processedCount++;
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit VectorWorker(osmscout::ProcessingQueue<VectorData>& queue)
    : queue(queue),
      thread(&VectorWorker::ProcessorLoop,this),
      processedCount(0)
  {
  }

  void Wait() {
    thread.join();
  }
};

int main(int /*argc*/, char* /*argv*/[])
{
  std::cout << "Main thread id: #" << std::this_thread::get_id() << std::endl;
  std::cout << iterationCount << " iterations, every task takes " << taskDuration.count() << "ms" << std::endl;
  std::cout << std::endl;

  {
    std::cout << ">>> IntWorker..." << std::endl;

    osmscout::StopClock            stopClock;
    osmscout::ProcessingQueue<int> queue(1000);
    IntWorker                      worker(queue);

    std::cout << "Pushing int tasks..." << std::endl;

    for (size_t i=1; i<=iterationCount; i++) {
      queue.PushTask(i);
    }

    queue.PushTask(-1);

    queue.Stop();

    std::cout << "Pushing tasks...done, waiting..." << std::endl;

    worker.Wait();

    stopClock.Stop();
    std::cout << "#processed: " << worker.processedCount << std::endl;
    std::cout << "<<< IntWorker...done: " << stopClock.ResultString() << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << ">>> VectorWorker..." << std::endl;
    osmscout::StopClock                   stopClock;
    osmscout::ProcessingQueue<VectorData> queue(1000);
    VectorWorker                          worker(queue);

    std::cout << "Pushing vector tasks..." << std::endl;

    for (size_t i=1; i<=iterationCount; i++) {
      VectorData data(i);

      data.data.assign(i,i);

      queue.PushTask(std::move(data));
    }

    queue.PushTask(VectorData(-1));

    queue.Stop();

    std::cout << "Pushing tasks...done, waiting..." << std::endl;

    worker.Wait();

    stopClock.Stop();
    std::cout << "#processed: " << worker.processedCount << std::endl;
    std::cout << "<<< VectorWorker...done: " << stopClock.ResultString() << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << ">>> MultiVectorWorker..." << std::endl;
    osmscout::StopClock                   stopClock;
    osmscout::ProcessingQueue<VectorData> queue(1000);
    VectorWorker                          worker1(queue);
    VectorWorker                          worker2(queue);
    VectorWorker                          worker3(queue);
    VectorWorker                          worker4(queue);

    std::cout << "Pushing vector tasks..." << std::endl;

    for (size_t i=1; i<=iterationCount; i++) {
      VectorData data(i);

      data.data.assign(i,i);

      queue.PushTask(std::move(data));
    }

    queue.PushTask(VectorData(-1));

    queue.Stop();

    std::cout << "Pushing tasks...done, waiting..." << std::endl;

    worker1.Wait();
    worker2.Wait();
    worker3.Wait();
    worker4.Wait();

    stopClock.Stop();

    std::cout << "#processed: "
              << worker1.processedCount+worker2.processedCount+worker3.processedCount+
                 worker4.processedCount << std::endl;
    std::cout << "<<< MultiVectorWorker...done: " << stopClock.ResultString() << std::endl;
    std::cout << std::endl;
  }

  return 0;
}
