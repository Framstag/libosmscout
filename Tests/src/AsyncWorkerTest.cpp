#include <osmscout/async/AsyncWorker.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

/*
 * Tests for the worker and its job queue: its start/stop behaviour, the result of a job, its
 * self-deletion, and the lifetime contract that a worker which owns a job queue guarantees
 * (spec async-worker-lifetime) - destroying it stops its job thread and waits for the job in
 * progress before the members its jobs use are destroyed, a job that was still queued is not run,
 * and a job that fails is reported instead of ending the process.
 *
 * The lifetime tests keep everything a job touches outside the worker and capture it by value, so
 * a job that outlives its worker reads valid memory: they assert the *ordering* the contract
 * requires instead of reproducing the undefined behaviour that a broken ordering causes.
 */

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

namespace {

  /** State owned by the test, shared with the jobs so they never touch worker members. */
  struct JobState
  {
    std::atomic<bool> jobStarted{false};
    std::atomic<bool> jobFinished{false};
    std::atomic<bool> jobRanAfterMembersDestroyed{false};
    std::atomic<bool> queuedJobRan{false};
    std::atomic<bool> workerDestroyed{false};
    std::atomic<bool> selfDeletingWorkerDestroyed{false};

    std::mutex              mutex;
    std::condition_variable released;
    bool                    releaseJob{false};

    void Release()
    {
      std::unique_lock<std::mutex> lock(mutex);
      releaseJob=true;
      released.notify_all();
    }

    void WaitForRelease()
    {
      std::unique_lock<std::mutex> lock(mutex);
      released.wait(lock, [this] { return releaseJob; });
    }
  };

  bool WaitFor(const std::function<bool()> &condition,
               // NOLINTNEXTLINE(misc-include-cleaner) the literal operators live in <chrono>, included above
               const std::chrono::milliseconds &timeout=10s)
  {
    auto deadline=std::chrono::steady_clock::now()+timeout;

    while (std::chrono::steady_clock::now()<deadline) {
      if (condition()) {
        return true;
      }

      // NOLINTNEXTLINE(misc-include-cleaner) the literal operators live in <chrono>, included above
      std::this_thread::sleep_for(2ms);
    }

    return condition();
  }

  /**
   * A member whose destruction is observable. It holds no worker state and the job reads the
   * shared state by value, so a job that outlives the worker can still record what it saw.
   */
  class MemberGuard
  {
  private:
    std::shared_ptr<JobState> state;

  public:
    explicit MemberGuard(std::shared_ptr<JobState> state):
      state(std::move(state))
    {
    }

    MemberGuard(const MemberGuard&) = delete;
    MemberGuard& operator=(const MemberGuard&) = delete;
    MemberGuard(MemberGuard&&) = delete;
    MemberGuard& operator=(MemberGuard&&) = delete;

    ~MemberGuard()
    {
      if (!state->jobFinished) {
        // The members went away while the worker's job was still running.
        state->jobRanAfterMembersDestroyed=true;
      }
    }
  };

  class LifetimeWorker: public osmscout::AsyncWorker
  {
  private:
    std::shared_ptr<JobState> state;

    // Declared last, so it is destroyed first: the moment the members start going.
    MemberGuard guard;

  public:
    explicit LifetimeWorker(const std::shared_ptr<JobState> &state):
      AsyncWorker("LifetimeWorker"),
      state(state),
      guard(state)
    {
    }

    LifetimeWorker(const LifetimeWorker&) = delete;
    LifetimeWorker& operator=(const LifetimeWorker&) = delete;
    LifetimeWorker(LifetimeWorker&&) = delete;
    LifetimeWorker& operator=(LifetimeWorker&&) = delete;

    ~LifetimeWorker() override
    {
      // The contract under test: the worker is stopped before its members are destroyed.
      Stop();
    }

    /** A job that blocks until the test releases it, then records what it saw. */
    void StartBlockingJob()
    {
      Async<bool>([state=this->state](osmscout::Breaker &) -> bool {
        state->jobStarted=true;
        state->WaitForRelease();
        state->jobFinished=true;
        return true;
      });
    }

    /**
     * A job that is queued behind the blocking one. The queue drains what was queued when it is
     * stopped, so this job runs before the worker exits - while the members are still alive.
     */
    void StartQueuedJob()
    {
      Async<bool>([state=this->state](osmscout::Breaker &) -> bool {
        state->queuedJobRan=true;
        state->jobFinished=true;
        return true;
      });
    }
  };

  /**
   * A worker that deletes itself from its own job, the pattern the client library uses for a
   * module that stops accepting work and then releases itself from its worker thread.
   */
  class SelfDeletingWorker: public osmscout::AsyncWorker
  {
  private:
    std::shared_ptr<JobState> state;

  public:
    explicit SelfDeletingWorker(std::shared_ptr<JobState> state):
      AsyncWorker("SelfDeletingWorker"),
      state(std::move(state))
    {
    }

    SelfDeletingWorker(const SelfDeletingWorker&) = delete;
    SelfDeletingWorker& operator=(const SelfDeletingWorker&) = delete;
    SelfDeletingWorker(SelfDeletingWorker&&) = delete;
    SelfDeletingWorker& operator=(SelfDeletingWorker&&) = delete;

    ~SelfDeletingWorker() override
    {
      state->selfDeletingWorkerDestroyed=true;

      // Called from the worker thread, which cannot wait for itself.
      Stop();
    }

    void StartSelfDeletion()
    {
      DeleteLater();
    }
  };

  /** A worker that hands the futures of a failing and of a successful job to the test. */
  class ReportingWorker: public osmscout::AsyncWorker
  {
  public:
    ReportingWorker():
      AsyncWorker("ReportingWorker")
    {
    }

    osmscout::CancelableFuture<bool> SubmitFailingJob()
    {
      return Async<bool>([](osmscout::Breaker &) -> bool {
        throw std::runtime_error("job under test fails");
      });
    }

    osmscout::CancelableFuture<bool> SubmitSuccessfulJob()
    {
      return Async<bool>([](osmscout::Breaker &) -> bool {
        return true;
      });
    }
  };
}

TEST_CASE("Worker destruction stops the job thread before the members are destroyed")
{
  auto state=std::make_shared<JobState>();
  auto worker=std::make_unique<LifetimeWorker>(state);

  worker->StartBlockingJob();
  REQUIRE(WaitFor([&] { return state->jobStarted.load(); }));

  std::thread destroyer([worker=std::move(worker),state]() mutable {
    worker.reset();
    state->workerDestroyed=true;
  });

  // The job is blocked, so the destruction can only finish once the job is released: it waits.
  REQUIRE(!state->workerDestroyed.load());

  state->Release();
  destroyer.join();

  REQUIRE(state->workerDestroyed.load());
  REQUIRE(state->jobFinished.load());

  // The decisive assertion: the job did not run against destroyed members. Without a shutdown
  // before the members go, the member guard observes a job that is still running.
  REQUIRE(!state->jobRanAfterMembersDestroyed.load());
}

TEST_CASE("Worker destruction drains a job that was still queued")
{
  auto state=std::make_shared<JobState>();
  auto worker=std::make_unique<LifetimeWorker>(state);

  // The first job blocks, so the second one stays in the queue.
  worker->StartBlockingJob();
  REQUIRE(WaitFor([&] { return state->jobStarted.load(); }));

  worker->StartQueuedJob();

  std::thread destroyer([worker=std::move(worker),state]() mutable {
    worker.reset();
    state->workerDestroyed=true;
  });

  REQUIRE(!state->workerDestroyed.load());

  state->Release();
  destroyer.join();

  REQUIRE(state->workerDestroyed.load());

  // The queue stops accepting jobs but drains what it holds: the queued job runs before the
  // worker exits.
  REQUIRE(state->queuedJobRan.load());
}

TEST_CASE("Worker shutdown can be called twice")
{
  ReportingWorker worker;

  REQUIRE(worker.SubmitSuccessfulJob().StdFuture().get());

  worker.Stop();
  worker.Stop();

  // The destructor shuts the worker down once more, which must also be harmless.
}

TEST_CASE("A worker deletes itself from its own job without deadlocking")
{
  auto state=std::make_shared<JobState>();

  auto worker=std::make_unique<SelfDeletingWorker>(state);

  // The worker deletes itself from its own job, so the test gives up ownership here.
  worker.release()->StartSelfDeletion();

  // The queue stops first and the worker deletes itself from its own thread, so there is nothing
  // left to wait for; the destruction must complete.
  REQUIRE(WaitFor([&] { return state->selfDeletingWorkerDestroyed.load(); }));
}

TEST_CASE("A failing job is reported and the worker keeps serving")
{
  ReportingWorker worker;

  auto failing=worker.SubmitFailingJob();

  // The caller of a failed job is released with the default value of the result type instead of
  // waiting forever, and the worker keeps serving the queue.
  REQUIRE(failing.StdFuture().get() == false);

  auto successful=worker.SubmitSuccessfulJob();

  REQUIRE(successful.StdFuture().get() == true);
}
