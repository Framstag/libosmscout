/*
  WorkQueue - a test program for libosmscout
  Copyright (C) 2015  Tim Teulings

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

#include <osmscout/async/WorkQueue.h>

#include <catch2/catch_test_macros.hpp>

class Worker
{
private:
  osmscout::WorkQueue<int> queue;
  std::thread              worker;

private:
  void TaskLoop()
  {
    while (!queue.Finished()) {
      if (auto optionalTask=queue.PopTask(); optionalTask) {
        optionalTask.value()();
      }
    }
  }

public:
  Worker()
  : worker(&Worker::TaskLoop,this)
  {

  }

  ~Worker()
  {
    worker.join();
  }

  std::future<int> PushWork(int a, int b)
  {
    std::packaged_task<int()> task([a,b] {

      return a+b;
    });

    std::future<int> future=task.get_future();

    queue.PushTask(std::move(task));

    return future;
  }

  void Stop()
  {
    queue.Stop();
  }
};

TEST_CASE("WorkQueue")
{
  Worker                        worker;
  std::vector<std::future<int>> futures;

  for (int i=1; i<=100; i++) {
    futures.push_back(worker.PushWork(i,i*2));
  }

  for (size_t i=0; i<futures.size(); i++) {
    int expected=static_cast<int>(i+1)+(static_cast<int>(i+1)*2);
    REQUIRE(futures[i].get()==expected);
  }

  worker.Stop();
  std::this_thread::sleep_for (std::chrono::seconds(1));
}