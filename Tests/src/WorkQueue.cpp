/*
  ThreadedDatabase - a test program for libosmscout
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

#include <osmscout/util/WorkQueue.h>

class Worker
{
private:
  std::thread              worker;
  osmscout::WorkQueue<int> queue;

private:
  int Work(int a, int b)
  {
    std::cout << "Doing task #" << a << std::endl;

    return a+b;
  }

  void TaskLoop()
  {
    std::packaged_task<int()> task;

    std::cout << "Starting TaskLoop()..." << std::endl;

    while (queue.PopTask(task)) {
      task();
    }

    std::cout << "Quit TaskLoop()" << std::endl;
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
    std::packaged_task<int()> task(std::bind(&Worker::Work,this,a,b));

    std::future<int> future=task.get_future();

    queue.PushTask(task);

    return future;
  }

  void Stop()
  {
    queue.Stop();
  }
};

int main(int /*argc*/, char* /*argv*/[])
{
  Worker                        worker;
  std::vector<std::future<int>> futures;

  std::cout << std::this_thread::get_id() << ": Pushing work..." << std::endl;
  for (int i=1; i<=100; i++) {
    std::cout << "Pushing task #" << i << std::endl;
    futures.push_back(worker.PushWork(i,i*2));
  }

  std::cout << "Waiting for futures..." << std::endl;

  for (auto& future : futures) {
    future.wait();

    std::cout << "Result: " << future.get() << std::endl;
  }

  std::cout << "Signaling worker stop..." << std::endl;
  worker.Stop();

  std::cout << "Waiting for 1 seconds..." << std::endl;

  std::this_thread::sleep_for (std::chrono::seconds(1));

  std::cout << "Bye" << std::endl;

  return 0;
}
