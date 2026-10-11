#include <osmscoutclient/MapManager.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {

using namespace std::chrono_literals;

/** Records the database sets published by a scan. The scan publishes from its worker thread. */
class PublicationLog
{
public:
  osmscout::Slot<std::vector<std::filesystem::path>> slot;

  PublicationLog():
    slot([this](const std::vector<std::filesystem::path> &directories) {
      std::unique_lock<std::mutex> lock(mutex);
      publications.push_back(directories);
    })
  {
    // no code
  }

  size_t Count() const
  {
    std::unique_lock<std::mutex> lock(mutex);
    return publications.size();
  }

  std::vector<std::filesystem::path> At(size_t index) const
  {
    std::unique_lock<std::mutex> lock(mutex);
    if (index>=publications.size()) {
      return {};
    }
    return publications[index];
  }

private:
  mutable std::mutex                              mutex;
  std::vector<std::vector<std::filesystem::path>> publications;
};

/** Temporary directory tree that removes itself. */
class TempDir
{
public:
  explicit TempDir(const std::string &name)
  {
    static std::atomic<unsigned int> counter{0};

    dir=std::filesystem::temp_directory_path()/
        ("osmscout-mapmanager-"+name+"-"+std::to_string(counter++));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir);
  }

  ~TempDir()
  {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  const std::filesystem::path& Path() const
  {
    return dir;
  }

private:
  std::filesystem::path dir;
};

/** A directory that looks like a complete map database to MapDirectory::IsValid(). */
void CreateValidDatabase(const std::filesystem::path &dir)
{
  std::filesystem::create_directories(dir);

  for (const auto &fileName: osmscout::MapDirectory::MandatoryFiles()) {
    std::ofstream file(dir/fileName, std::ios::binary);
  }
}

/** A directory with a database file, but without the remaining mandatory files. */
void CreateIncompleteDatabase(const std::filesystem::path &dir)
{
  std::filesystem::create_directories(dir);

  std::ofstream file(dir/"types.dat", std::ios::binary);
}

/** Filler entries, so that a scan of this tree takes more than an instant. */
void CreateNoise(const std::filesystem::path &dir,
                 size_t directoryCount,
                 size_t fileCount)
{
  std::filesystem::create_directories(dir);

  for (size_t d=0; d<directoryCount; d++) {
    auto sub=dir/("directory-"+std::to_string(d));
    std::filesystem::create_directories(sub);

    for (size_t f=0; f<fileCount; f++) {
      std::ofstream file(sub/("file-"+std::to_string(f)+".dat"), std::ios::binary);
    }
  }
}

bool WaitFor(osmscout::CancelableFuture<bool> future,
             std::chrono::milliseconds timeout=30s)
{
  auto done=std::make_shared<std::promise<void>>();

  future.OnComplete([done](bool) {
    done->set_value();
  });

  return done->get_future().wait_for(timeout)==std::future_status::ready;
}

bool WaitForPublications(const PublicationLog &log,
                         size_t count,
                         std::chrono::milliseconds timeout=30s)
{
  auto deadline=std::chrono::steady_clock::now()+timeout;

  while (log.Count()<count && std::chrono::steady_clock::now()<deadline) {
    std::this_thread::sleep_for(5ms);
  }

  return log.Count()>=count;
}

osmscout::MapManagerRef MakeManager(const std::vector<std::filesystem::path> &directories)
{
  return std::make_shared<osmscout::MapManager>(directories);
}

}

TEST_CASE("A scan publishes the valid databases of all registered directories in one set") {
  TempDir base("publish");

  CreateValidDatabase(base.Path()/"db-one");
  CreateIncompleteDatabase(base.Path()/"incomplete");
  CreateValidDatabase(base.Path()/"nested"/"db-two");

  auto            manager=MakeManager({base.Path()});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  REQUIRE(WaitFor(manager->LookupDatabases()));

  REQUIRE(manager->GetDatabaseDirectories().size()==2);

  // One publication, with the complete set - not one publication per directory.
  REQUIRE(log.Count()==1);
  REQUIRE(log.At(0).size()==2);
}

TEST_CASE("A database reachable through two registered directories is published once") {
  TempDir base("duplicate");

  CreateValidDatabase(base.Path()/"db-one");
  CreateValidDatabase(base.Path()/"nested"/"db-two");

  auto            manager=MakeManager({base.Path(), base.Path()/"nested"});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  REQUIRE(WaitFor(manager->LookupDatabases()));

  REQUIRE(manager->GetDatabaseDirectories().size()==2);
  REQUIRE(log.Count()==1);
  REQUIRE(log.At(0).size()==2);
}

TEST_CASE("A registered directory that is not readable does not end the scan") {
  TempDir base("unreadable");

  CreateValidDatabase(base.Path()/"db-one");

  auto            manager=MakeManager({base.Path()/"does-not-exist", base.Path()});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  REQUIRE(WaitFor(manager->LookupDatabases()));

  REQUIRE(manager->GetDatabaseDirectories().size()==1);
  REQUIRE(log.Count()==1);
  REQUIRE(log.At(0).size()==1);
}

TEST_CASE("A scan without a database publishes an empty set") {
  TempDir base("empty");

  CreateIncompleteDatabase(base.Path()/"incomplete");

  auto            manager=MakeManager({base.Path()});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  REQUIRE(WaitFor(manager->LookupDatabases()));

  REQUIRE(manager->GetDatabaseDirectories().empty());
  REQUIRE(log.Count()==1);
  REQUIRE(log.At(0).empty());
}

TEST_CASE("A stopped scan publishes nothing and the next scan publishes the complete set") {
  TempDir base("stopped");

  CreateValidDatabase(base.Path()/"db-one");
  CreateNoise(base.Path()/"noise", 50, 20);

  auto            manager=MakeManager({base.Path()});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  auto stopped=manager->LookupDatabases();

  stopped.Cancel();

  // The next scan is serialized behind the stopped one, so its publication means
  // that the stopped scan has ended.
  REQUIRE(WaitFor(manager->LookupDatabases()));

  REQUIRE(stopped.IsCanceled());

  // Only the second scan published, and it published the complete set.
  REQUIRE(log.Count()==1);
  REQUIRE(log.At(0).size()==1);
}

TEST_CASE("A directory registered during a scan is not part of that scan") {
  TempDir base("registered");

  auto dirA=base.Path()/"registered-first";
  auto dirB=base.Path()/"registered-later";

  CreateValidDatabase(dirA/"db-one");
  CreateNoise(dirA/"noise", 50, 20);
  CreateValidDatabase(dirB/"db-two");

  auto            manager=MakeManager({dirA});
  PublicationLog  log;

  manager->databaseListChanged.Connect(log.slot);

  manager->LookupDatabases();

  // Let the scan start, so that the registration below really happens while it runs.
  std::this_thread::sleep_for(100ms);

  manager->AddLookupDirectory(dirB);

  // AddLookupDirectory waits for the running scan and then starts a new one.
  REQUIRE(WaitForPublications(log, 2));

  REQUIRE(log.Count()==2);
  // The scan that was running when the directory was registered saw only the first one.
  REQUIRE(log.At(0).size()==1);
  // The scan started after the registration saw both.
  REQUIRE(log.At(1).size()==2);
  REQUIRE(manager->GetDatabaseDirectories().size()==2);
}

TEST_CASE("Destroying the manager while a scan runs is safe, repeatedly") {
  TempDir base("teardown");

  CreateValidDatabase(base.Path()/"db-one");
  CreateNoise(base.Path()/"noise", 20, 20);

  PublicationLog log;

  for (int i=0; i<10; i++) {
    auto manager=MakeManager({base.Path()});

    manager->databaseListChanged.Connect(log.slot);
    manager->LookupDatabases();

    // Destruction stops the scan (or discards it before it starts) instead of
    // letting it run against released state.
    manager.reset();

    // A scan that outran the teardown and published while the manager was still
    // alive is not a defect, so the count is taken after the teardown. What must
    // hold is that no publication follows it: the signal died with the manager, so
    // a scan the teardown stopped can only stay silent, and a publication on behalf
    // of one would grow the count or fault.
    size_t publicationsAfterTeardown=log.Count();

    std::this_thread::sleep_for(10ms);

    REQUIRE(log.Count()==publicationsAfterTeardown);
  }
}
