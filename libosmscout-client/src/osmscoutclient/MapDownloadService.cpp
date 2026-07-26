/*
  This source is part of the libosmscout library
  Copyright (C) 2025  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <osmscoutclient/MapDownloadService.h>

#include <osmscoutclient/json/json.hpp>

#include <osmscout/TypeConfig.h>
#include <osmscout/log/Logger.h>

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

namespace osmscout {

MapDownloadService::MapDownloadService(MapManagerRef mapManager,
                                         SettingsRef settings)
  : AsyncWorker("MapDownloadService")
  , mapManager(mapManager)
  , settings(settings)
{
}

MapDownloadService::~MapDownloadService()
{
  // AsyncWorker base destructor handles queue.Stop() and thread.join()
}

CancelableFuture<std::vector<AvailableMapEntry>> MapDownloadService::FetchMapList(
    const MapProvider &provider,
    HttpClient &httpClient)
{
  return Async<std::vector<AvailableMapEntry>>(
      [this, &provider, &httpClient](Breaker &breaker) -> std::vector<AvailableMapEntry> {
        if (breaker.IsAborted()) {
          return {};
        }

        // Use FILE_FORMAT_VERSION for both bounds since
        // MIN_FORMAT_VERSION == MAX_FORMAT_VERSION == FILE_FORMAT_VERSION
        std::string uri = provider.getListUri(FILE_FORMAT_VERSION,
                                               FILE_FORMAT_VERSION,
                                               "en");

        osmscout::log.Info() << "Fetching map list from " << uri;

        std::string jsonStr = httpClient.Fetch(uri);
        if (jsonStr.empty()) {
          osmscout::log.Error() << "Failed to fetch map list from " << uri;
          return {};
        }

        if (breaker.IsAborted()) {
          return {};
        }

        try {
          auto json = nlohmann::json::parse(jsonStr);
          return AvailableMapEntry::FromJsonArray(json, provider);
        } catch (const nlohmann::json::exception &e) {
          osmscout::log.Error() << "Failed to parse map list JSON: " << e.what();
          return {};
        }
      });
}

bool MapDownloadService::DownloadMapSync(
    const AvailableMapEntry &entry,
    const std::filesystem::path &targetDir,
    HttpClient &httpClient,
    MapManagerRef mapManager,
    const std::function<void(uint64_t, uint64_t)> &progress)
{
  std::vector<DownloadJobState> jobs;
  std::mutex jobsMutex;
  osmscout::ThreadedBreaker breaker;
  return DownloadMapInternal(jobs, jobsMutex, mapManager,
                             entry, targetDir, httpClient, breaker, progress);
}

CancelableFuture<bool> MapDownloadService::DownloadMap(
    const AvailableMapEntry &entry,
    const std::filesystem::path &targetDir,
    HttpClient &httpClient,
    const std::function<void(uint64_t, uint64_t)> &progress)
{
  return Async<bool>(
      [this, entry, targetDir, &httpClient, &progress](Breaker &breaker) -> bool {
        return DownloadMapInternal(jobs, jobsMutex, mapManager,
                                   entry, targetDir, httpClient, breaker, progress);
      });
}

void MapDownloadService::CancelDownload(const std::filesystem::path &targetDir)
{
  std::unique_lock<std::mutex> lock(jobsMutex);
  for (auto &job : jobs) {
    if (job.targetDir == targetDir && !job.done) {
      job.error = "Cancelled by user";
      job.done = true;
      // The Breaker mechanism in AsyncWorker handles actual cancellation
      // of the running task. We just mark the state here.
    }
  }
}

std::vector<DownloadJobState> MapDownloadService::GetDownloads() const
{
  std::unique_lock<std::mutex> lock(jobsMutex);
  return jobs;
}

bool MapDownloadService::DownloadMapInternal(std::vector<DownloadJobState> &jobs,
                                             std::mutex &jobsMutex,
                                             MapManagerRef mapManager,
                                             const AvailableMapEntry &entry,
                                             const std::filesystem::path &targetDir,
                                             HttpClient &httpClient,
                                             Breaker &breaker,
                                             const std::function<void(uint64_t, uint64_t)> &progress)
{
  if (entry.IsDirectory()) {
    osmscout::log.Error() << "Cannot download a directory entry: " << entry.GetName();
    return false;
  }

  if (breaker.IsAborted()) {
    return false;
  }

  // Register job state
  DownloadJobState jobState;
  jobState.mapName = entry.GetName();
  jobState.mapPath = entry.GetPath();
  jobState.targetDir = targetDir;
  jobState.totalBytes = entry.GetSize();
  {
    std::unique_lock<std::mutex> lock(jobsMutex);
    jobs.push_back(jobState);
  }

  // Create target directory
  std::error_code ec;
  bool dirCreated = std::filesystem::create_directories(targetDir, ec);
  if (!dirCreated && ec) {
    std::unique_lock<std::mutex> lock(jobsMutex);
    for (auto &j : jobs) {
      if (j.targetDir == targetDir) {
        j.error = ec.message();
        j.done = true;
      }
    }
    return false;
  }

  // Check for existing partial download — clean up if needed
  // Only check if directory has metadata file (indicates previous download attempt)
  std::string dirStr = targetDir.string();
  std::string metaPathStr = dirStr + "/" + MapDirectory::FileMetadata;
  bool hasMetaFile = (access(metaPathStr.c_str(), F_OK) == 0);
  if (hasMetaFile) {
    // Partial download exists — clean it up by removing all files
    for (const auto &f : MapDirectory::MandatoryFiles()) {
      std::string fp = dirStr + "/" + f;
      std::string tp = dirStr + "/" + f + MapDirectory::TemporaryFileSuffix;
      remove(fp.c_str());
      remove(tp.c_str());
    }
    for (const auto &f : MapDirectory::OptionalFiles()) {
      std::string fp = dirStr + "/" + f;
      std::string tp = dirStr + "/" + f + MapDirectory::TemporaryFileSuffix;
      remove(fp.c_str());
      remove(tp.c_str());
    }
    remove(metaPathStr.c_str());
  }

  // Write metadata.json first (marks this as a download in progress)
  nlohmann::json metadata;
  metadata["name"] = entry.GetName();
  {
    std::string pathStr;
    for (size_t i = 0; i < entry.GetPath().size(); ++i) {
      if (i > 0) pathStr += "/";
      pathStr += entry.GetPath()[i];
    }
    metadata["map"] = pathStr;
  }
  metadata["version"] = entry.GetVersion();
  metadata["creation"] = std::chrono::duration_cast<std::chrono::seconds>(
      entry.GetCreation().time_since_epoch()).count();

  std::string metadataPathStr = dirStr + "/" + MapDirectory::FileMetadata;
  std::string metaJson = metadata.dump(2);
  {
    int fd = open(metadataPathStr.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
      osmscout::log.Error() << "Failed to write metadata to " << metadataPathStr
                            << ": " << strerror(errno);
      std::unique_lock<std::mutex> lock(jobsMutex);
      for (auto &j : jobs) {
        if (j.targetDir == targetDir) {
          j.error = "Failed to write metadata";
          j.done = true;
        }
      }
      return false;
    }
    (void)write(fd, metaJson.c_str(), metaJson.size());
    close(fd);
  }

  // Collect all files to download
  std::vector<std::string> allFiles = MapDirectory::MandatoryFiles();
  std::vector<std::string> optFiles = MapDirectory::OptionalFiles();
  allFiles.insert(allFiles.end(), optFiles.begin(), optFiles.end());

  // Build server base path
  std::string serverBase = entry.GetProvider().getUri() + "/" + entry.GetServerDirectory();

  // Download each file
  uint64_t totalDownloaded = 0;
  bool allSucceeded = true;

  for (const auto &fileName : allFiles) {
    if (breaker.IsAborted()) {
      allSucceeded = false;
      break;
    }

    std::string fileUrl = serverBase + "/" + fileName;
    std::filesystem::path tempPath = targetDir / (fileName + MapDirectory::TemporaryFileSuffix);
    std::filesystem::path finalPath = targetDir / fileName;

    // Update job state
    {
      std::unique_lock<std::mutex> lock(jobsMutex);
      for (auto &j : jobs) {
        if (j.targetDir == targetDir) {
          j.currentFile = fileName;
        }
      }
    }

    osmscout::log.Debug() << "Downloading " << fileUrl << " to " << tempPath.string();

    // Download with progress callback
    bool fileOk = httpClient.Download(
        fileUrl, tempPath,
        [&jobs, &jobsMutex, &targetDir, &totalDownloaded, &breaker, &progress](uint64_t bytes, uint64_t total) -> bool {
          if (breaker.IsAborted()) {
            return false;
          }
          uint64_t bytesDownloaded = totalDownloaded + bytes;
          {
            std::unique_lock<std::mutex> lock(jobsMutex);
            for (auto &j : jobs) {
              if (j.targetDir == targetDir) {
                j.bytesDownloaded = bytesDownloaded;
                if (total > 0) {
                  j.totalBytes = total;
                }
              }
            }
          }
          if (progress) {
            progress(bytesDownloaded, total);
          }
          return true;
        });

    if (!fileOk) {
      osmscout::log.Error() << "Failed to download " << fileUrl;
      allSucceeded = false;
      break;
    }

    // Rename .download → final
    std::filesystem::rename(tempPath, finalPath, ec);
    if (ec) {
      osmscout::log.Error() << "Failed to rename " << tempPath.string()
                            << " to " << finalPath.string() << ": " << ec.message();
      allSucceeded = false;
      break;
    }

    // Update total downloaded
    std::error_code sizeEc;
    auto fileSize = std::filesystem::file_size(finalPath, sizeEc);
    if (!sizeEc) {
      totalDownloaded += fileSize;
    }
  }

  // Update final job state
  {
    std::unique_lock<std::mutex> lock(jobsMutex);
    for (auto &j : jobs) {
      if (j.targetDir == targetDir) {
        j.done = true;
        j.successful = allSucceeded;
        j.bytesDownloaded = totalDownloaded;
        if (!allSucceeded && j.error.empty()) {
          j.error = "Download failed or cancelled";
        }
      }
    }
  }

  if (!allSucceeded) {
    // Clean up partial download - remove files individually
    std::error_code ec;
    for (const auto &fileName : allFiles) {
      std::filesystem::path filePath = targetDir / fileName;
      std::filesystem::remove(filePath, ec);
      std::filesystem::remove(targetDir / (fileName + MapDirectory::TemporaryFileSuffix), ec);
    }
    std::filesystem::remove(metadataPathStr, ec);
    std::filesystem::remove(targetDir, ec);
    return false;
  }

  // Register the new map directory with MapManager
  if (mapManager) {
    mapManager->AddLookupDirectory(targetDir);
    // Don't trigger LookupDatabases here — it runs async on MapManager's thread
    // and would race with test teardown. Callers should trigger it explicitly.
  }

  osmscout::log.Info() << "Successfully downloaded map " << entry.GetName()
                       << " to " << targetDir.string();
  return true;
}

std::vector<std::string> MapDownloadService::MapFiles()
{
  std::vector<std::string> files = MapDirectory::MandatoryFiles();
  std::vector<std::string> optFiles = MapDirectory::OptionalFiles();
  files.insert(files.end(), optFiles.begin(), optFiles.end());
  return files;
}

bool MapDownloadService::PrepareMapDirectory(const AvailableMapEntry &entry,
                                              const std::filesystem::path &targetDir)
{
  if (entry.IsDirectory()) {
    osmscout::log.Error() << "Cannot prepare directory for a directory entry: " << entry.GetName();
    return false;
  }

  // Create target directory
  std::error_code ec;
  std::filesystem::create_directories(targetDir, ec);
  if (ec) {
    osmscout::log.Error() << "Failed to create target directory " << targetDir.string()
                          << ": " << ec.message();
    return false;
  }

  std::string dirStr = targetDir.string();

  // Clean up files left over from a previous download attempt
  auto cleanupFile = [&dirStr](const std::string &fileName) {
    std::string fp = dirStr + "/" + fileName;
    std::string tp = dirStr + "/" + fileName + MapDirectory::TemporaryFileSuffix;
    std::error_code err;
    std::filesystem::remove(fp, err);
    std::filesystem::remove(tp, err);
  };

  for (const auto &f : MapDirectory::MandatoryFiles()) {
    cleanupFile(f);
  }
  for (const auto &f : MapDirectory::OptionalFiles()) {
    cleanupFile(f);
  }
  {
    std::error_code err;
    std::filesystem::remove(dirStr + "/" + MapDirectory::FileMetadata, err);
  }

  // Write metadata.json
  nlohmann::json metadata;
  metadata["name"] = entry.GetName();
  {
    std::string pathStr;
    for (size_t i = 0; i < entry.GetPath().size(); ++i) {
      if (i > 0) pathStr += "/";
      pathStr += entry.GetPath()[i];
    }
    metadata["map"] = pathStr;
  }
  metadata["version"] = entry.GetVersion();
  metadata["creation"] = std::chrono::duration_cast<std::chrono::seconds>(
      entry.GetCreation().time_since_epoch()).count();

  std::string metadataPathStr = dirStr + "/" + MapDirectory::FileMetadata;
  std::string metaJson = metadata.dump(2);
  int fd = open(metadataPathStr.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    osmscout::log.Error() << "Failed to write metadata to " << metadataPathStr
                          << ": " << strerror(errno);
    return false;
  }
  (void)write(fd, metaJson.c_str(), metaJson.size());
  close(fd);

  return true;
}

bool MapDownloadService::RegisterMapDirectory(const std::filesystem::path &targetDir,
                                               MapManagerRef mapManager)
{
  if (!mapManager) {
    osmscout::log.Error() << "Cannot register map directory without a MapManager";
    return false;
  }

  mapManager->AddLookupDirectory(targetDir);
  osmscout::log.Info() << "Registered map directory " << targetDir.string();
  return true;
}

}
