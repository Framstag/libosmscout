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

#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include <osmscout/util/ProcessingQueue.h>
#include <osmscout/util/StopClock.h>

class IntWorker
{
private:
  osmscout::ProcessingQueue<int>& queue;
  std::thread                     thread;

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
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit IntWorker(osmscout::ProcessingQueue<int>& queue)
  : queue(queue),
    thread(&IntWorker::ProcessorLoop,this)
  {
  }

  ~IntWorker() {
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

  VectorData(VectorData&& other) noexcept
   : id(other.id),
     data(std::move(other.data))
  {
    //std::cout << "Move constructed vector " << id << std::endl;
  }

};

class VectorWorker
{
private:
  osmscout::ProcessingQueue<VectorData>& queue;
  std::thread                            thread;

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
    }

    std::cout << "Processing...done" << std::endl;
  }

public:
  explicit VectorWorker(osmscout::ProcessingQueue<VectorData>& queue)
    : queue(queue),
      thread(&VectorWorker::ProcessorLoop,this)
  {
  }

  ~VectorWorker() {
    thread.join();
  }
};

int main(int /*argc*/, char* /*argv*/[])
{
  //std::cout << "#" << std::this_thread::get_id() << " IntWorker..." << std::endl;
  osmscout::StopClock            stopClockInt;
  {
    osmscout::ProcessingQueue<int> queue(1000);
    IntWorker                      intWorker(queue);

    std::cout << "Pushing int tasks..." << std::endl;

    for (int i=1; i<=1000000; i++) {
      queue.PushTask(i);
    }

    queue.PushTask(-1);

    queue.Stop();

    std::cout << "Pushing tasks...done" << std::endl;
  }
  stopClockInt.Stop();
  std::cout << "IntWorker...done: " << stopClockInt.ResultString() << std::endl;

  //std::cout << "#" << std::this_thread::get_id() << " VectorWorker..." << std::endl;
  osmscout::StopClock                   stopClockVector;
  {
    osmscout::ProcessingQueue<VectorData> queue(1000);
    VectorWorker                          vectorWorker(queue);

    std::cout << "Pushing vector tasks..." << std::endl;

    for (int i=1; i<=100000; i++) {
      VectorData data(i);

      data.data.assign(i,i);

      queue.PushTask(std::move(data));
    }

    queue.PushTask(VectorData(-1));

    queue.Stop();

    std::cout << "Pushing tasks...done" << std::endl;
  }
  stopClockVector.Stop();
  std::cout << "VectorWorker...done: " << stopClockVector.ResultString() << std::endl;

  return 0;
}
