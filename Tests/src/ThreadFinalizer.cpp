#include <osmscout/async/Signal.h>
#include <osmscout/async/ThreadFinalizer.h>

#include <TestMain.h>

#include <thread>
#include <atomic>

TEST_CASE("Thread finalizer") {

  auto cnt=std::atomic_int(0);

  osmscout::Slot<std::thread::id> threadEndSlot([&cnt](const std::thread::id &id){
    std::cout << "Thread " << id << " ended" << std::endl;
    cnt.fetch_add(1);
  });

  REQUIRE(cnt==0);

  std::thread t1([&threadEndSlot](){
    std::cout << "ThreadFn " << std::this_thread::get_id() << std::endl;
    osmscout::threadFinalizer.threadExit.Connect(threadEndSlot);
  });

  t1.join();

  REQUIRE(cnt==1);
}
