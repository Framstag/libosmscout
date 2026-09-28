#include <osmscout/async/AsyncWorker.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/*
 * Tests for the worker and its job queue: its start/stop behaviour, the result of a job, its
 * self-deletion, and the lifetime contract that a worker which owns a job queue guarantees
 * (spec async-worker-lifetime) - destroying it stops its job thread before the members its jobs use
 * are destroyed, a job that was still queued is discarded and its caller released, a job that is in
 * flight is asked to stop and waited for, and a job that fails is reported instead of ending the
 * process.
 *
 * Most lifetime tests keep everything a job touches outside the worker and capture it by value, so a
 * job that outlives its worker reads valid memory: they assert the *ordering* the contract requires
 * instead of reproducing the undefined behaviour that a broken ordering causes. The last case instead
 * reproduces that access so that a sanitizer build reports it when the ordering is broken.
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

  // Nothing left in flight, so the destructor has nothing to report.
  worker.Stop();
}

TEST_CASE("Simple asynchronous computation") {
  TestWorker worker;
  REQUIRE(worker.Compute().StdFuture().get() == 42);
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
     * A job that is queued behind the blocking one. The queue stops and drops what it holds when
     * the worker is stopped, so this job does not run - and its caller is released by the
     * cancellation instead of waiting forever.
     */
    osmscout::CancelableFuture<bool> StartQueuedJob()
    {
      return Async<bool>([state=this->state](osmscout::Breaker &) -> bool {
        state->queuedJobRan=true;
        state->jobFinished=true;
        return true;
      });
    }

    /** A job that observes the stop request at its next check. */
    void StartPollingJob()
    {
      Async<bool>([state=this->state](osmscout::Breaker &breaker) -> bool {
        state->jobStarted=true;

        // Long enough that a teardown which waits for it is unmistakable.
        auto deadline=std::chrono::steady_clock::now()+
                      // NOLINTNEXTLINE(misc-include-cleaner) the literal operators live in <chrono>
                      5s;

        while (!breaker.IsAborted() && std::chrono::steady_clock::now()<deadline) {
          // NOLINTNEXTLINE(misc-include-cleaner) the literal operators live in <chrono>
          std::this_thread::sleep_for(1ms);
        }

        state->jobFinished=true;

        return breaker.IsAborted();
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

    /** A job queued in front of the deletion, to show that queued work runs before it. */
    void StartCountedJob()
    {
      Async<bool>([state=this->state](osmscout::Breaker &) -> bool {
        state->queuedJobRan=true;
        return true;
      });
    }

    void StartSelfDeletion()
    {
      DeleteLater();
    }
  };

  /** A worker that hands the futures of a failing, a successful and a counted job to the test. */
  class ReportingWorker: public osmscout::AsyncWorker
  {
  public:
    ReportingWorker():
      AsyncWorker("ReportingWorker")
    {
    }

    ~ReportingWorker() override
    {
      Stop();
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

    osmscout::CancelableFuture<bool> SubmitCountedJob(const std::shared_ptr<JobState> &state)
    {
      return Async<bool>([state](osmscout::Breaker &) -> bool {
        state->queuedJobRan=true;
        return true;
      });
    }
  };

  /** Sizes of the member buffers of LateJobWorker, so that releasing the members really releases memory. */
  constexpr size_t kLateDataSize=8;
  constexpr int    kLateDataValue=7;
  constexpr size_t kLateTextSize=64;

  /**
   * A worker whose job starts, sleeps and only then touches the owner's members, whose buffers live
   * on the heap. A build in which the worker is not stopped before its members are destroyed reports
   * the access below as a use-after-free, which is why this case is worth having next to the
   * ordering cases above.
   */
  class LateJobWorker: public osmscout::AsyncWorker
  {
  public:
    std::vector<int>                  data;
    std::string                       text;
    std::atomic<bool>                 started{false};
    std::shared_ptr<std::atomic<int>> touched;

    explicit LateJobWorker(std::shared_ptr<std::atomic<int>> touchedCounter):
      AsyncWorker("LateJobWorker"),
      touched(std::move(touchedCounter))
    {
      data.assign(kLateDataSize,kLateDataValue);
      text.assign(kLateTextSize,'x');

      Async<int>([this](osmscout::Breaker &) -> int {
        started=true;
        // NOLINTNEXTLINE(misc-include-cleaner) the literal operators live in <chrono>
        std::this_thread::sleep_for(300ms);
        data.front()++;
        text.push_back('y');
        (*touched)++;
        return 0;
      });
    }

    ~LateJobWorker() override
    {
      Stop();
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

TEST_CASE("Worker destruction discards a job that was still queued")
{
  auto state=std::make_shared<JobState>();
  auto worker=std::make_unique<LifetimeWorker>(state);

  // The first job blocks, so the second one stays in the queue.
  worker->StartBlockingJob();
  REQUIRE(WaitFor([&] { return state->jobStarted.load(); }));

  auto queued=worker->StartQueuedJob();

  std::thread destroyer([worker=std::move(worker),state]() mutable {
    worker.reset();
    state->workerDestroyed=true;
  });

  // Wait until the destruction has stopped the queue and cancelled the queued job: the queued job
  // cannot run after that, and releasing the running job cannot let it pick up the queued one.
  REQUIRE(WaitFor([&] { return queued.IsCanceled(); }));

  REQUIRE(!state->workerDestroyed.load());

  state->Release();
  destroyer.join();

  REQUIRE(state->workerDestroyed.load());

  // The queue stops and drops what it holds: the queued job does not run.
  REQUIRE(!state->queuedJobRan.load());

  // The caller of the discarded job is released instead of waiting for a job that never runs.
  REQUIRE(queued.IsCanceled());
  REQUIRE_THROWS_AS(queued.StdFuture().get(), std::runtime_error);
}

TEST_CASE("Teardown returns without waiting for a job that observes the stop request")
{
  auto state=std::make_shared<JobState>();
  auto worker=std::make_unique<LifetimeWorker>(state);

  worker->StartPollingJob();
  REQUIRE(WaitFor([&] { return state->jobStarted.load(); }));

  auto begin=std::chrono::steady_clock::now();
  worker.reset();
  auto elapsed=std::chrono::steady_clock::now()-begin;

  // The job would run for five seconds; the stop request ends it at its next check.
  REQUIRE(elapsed<2s);
  REQUIRE(state->jobFinished.load());
}

TEST_CASE("A job submitted after the worker stopped is cancelled, not run")
{
  auto state=std::make_shared<JobState>();

  ReportingWorker worker;

  worker.Stop();

  auto job=worker.SubmitCountedJob(state);

  std::this_thread::sleep_for(100ms);

  REQUIRE(!state->queuedJobRan.load());
  REQUIRE(job.IsCanceled());
}

TEST_CASE("A job in flight does not outlive the owner's state")
{
  auto touched=std::make_shared<std::atomic<int>>(0);

  {
    auto worker=std::make_unique<LateJobWorker>(touched);

    REQUIRE(WaitFor([&] { return worker->started.load(); }));

    // The job sleeps for 300ms and then writes to the members above: the destruction waits for it,
    // because it does not poll its breaker, so the members are still alive when it does.
    worker.reset();
  }

  REQUIRE(touched->load()==1);
}

TEST_CASE("Delete later")
{
  auto state=std::make_shared<JobState>();

  auto *worker=new SelfDeletingWorker(state);

  // The queued job runs before the deletion is served, and the worker then releases itself from its
  // own thread.
  worker->StartCountedJob();
  worker->StartSelfDeletion();
  worker=nullptr;

  REQUIRE(WaitFor([&] { return state->selfDeletingWorkerDestroyed.load(); }));
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
