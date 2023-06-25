
#include <osmscout/async/AsyncWorker.h>

#include <TestMain.h>

using namespace std::chrono_literals;

static auto   taskDuration=1s;

class TestWorker: public osmscout::AsyncWorker
{
public:
  TestWorker(): osmscout::AsyncWorker("Test")
  {
    // no code
  }

  ~TestWorker() override = default;

  osmscout::CancelableFuture<int> Compute()
  {
    return Async<int>([](osmscout::Breaker&){
      std::this_thread::sleep_for(taskDuration);
      return 42;
    });
  }
};


TEST_CASE("Start and stop") {
  TestWorker worker;
}

TEST_CASE("Simple asynchronous computation") {
  TestWorker worker;
  REQUIRE(worker.Compute().StdFuture().get() == 42);
}

TEST_CASE("Delete later") {
  auto worker=new TestWorker();
  worker->DeleteLater();
  worker=nullptr;
  std::this_thread::sleep_for(1s);
}
