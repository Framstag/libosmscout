#include <osmscout/async/Signal.h>
#include <osmscout/async/Thread.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <iostream>

TEST_CASE("Thread exit signal") {
  std::cout << "MainThread id: " << std::this_thread::get_id() << std::endl;

  auto cnt=std::atomic_int(0);

  osmscout::Slot<std::thread::id> threadEndSlot([&cnt](const std::thread::id &id){
    std::cout << "Thread " << id << " ended" << std::endl;
    cnt.fetch_add(1);
  });

  REQUIRE(cnt==0);

  std::cout << "Starting background thread..." << std::endl;
  std::thread t1([&threadEndSlot](){
    std::cout << "Background thread id: " << std::this_thread::get_id() << std::endl;
    std::cout << "Connect..." << std::endl;
    osmscout::ThreadExitSignal().Connect(threadEndSlot);
    std::cout << "Connect...done" << std::endl;
    std::cout << "Background thread...done" << std::endl;
  });

  std::cout << "Joining background thread..." << std::endl;
  t1.join();
  std::cout << "Joining background thread...done" << std::endl;

  std::cout << "Value of cnt: " << cnt << std::endl;

  REQUIRE(cnt==1);

  std::cout << "Test...done" << std::endl;
}
