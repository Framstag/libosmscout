#ifndef OSMSCOUT_CLIENT_MAPDOWNLOADSERVICE_H
#define OSMSCOUT_CLIENT_MAPDOWNLOADSERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutclient/ClientImportExport.h>
#include <osmscoutclient/AvailableMapEntry.h>
#include <osmscoutclient/HttpClient.h>
#include <osmscoutclient/MapDirectory.h>
#include <osmscoutclient/MapManager.h>
#include <osmscoutclient/Settings.h>

#include <osmscout/async/AsyncWorker.h>
#include <osmscout/async/CancelableFuture.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Represents the state of a single map download.
 */
struct OSMSCOUT_CLIENT_API DownloadJobState
{
  std::string mapName;
  std::vector<std::string> mapPath;
  std::filesystem::path targetDir;
  uint64_t bytesDownloaded{0};
  uint64_t totalBytes{0};
  std::string currentFile;
  std::string error;
  bool done{false};
  bool successful{false};
};

/**
 * \ingroup ClientAPI
 *
 * Service for downloading map databases from a map provider.
 * Runs on its own worker thread via AsyncWorker.
 * Uses the abstract HttpClient interface so no HTTP library dependency
 * is added to the core library.
 */
class OSMSCOUT_CLIENT_API MapDownloadService : public AsyncWorker
{
public:
  /**
   * @param mapManager  used to register downloaded maps
   * @param settings    used to read maps directory and provider config
   */
  MapDownloadService(MapManagerRef mapManager,
                     SettingsRef settings);

  ~MapDownloadService() override;

  MapDownloadService(const MapDownloadService&) = delete;
  MapDownloadService(MapDownloadService&&) = delete;
  MapDownloadService& operator=(const MapDownloadService&) = delete;
  MapDownloadService& operator=(MapDownloadService&&) = delete;

  /**
   * Fetch the list of available maps from a provider.
   *
   * @param provider   the map provider to query
   * @param httpClient HTTP client for the request
   * @return future containing the list of top-level entries
   */
  CancelableFuture<std::vector<AvailableMapEntry>> FetchMapList(
      const MapProvider &provider,
      HttpClient &httpClient);

  /**
   * Synchronous download of a map on the caller's thread, without creating
   * a background worker thread. This avoids JVM crashes on some Java 17
   * versions when a Java thread creates a long-lived native thread.
   *
   * @param entry      the map entry to download
   * @param targetDir  local directory to download into
   * @param httpClient HTTP client for the download
   * @param mapManager map manager to register the downloaded database
   * @param progress   optional callback invoked with bytes downloaded and total bytes
   * @return true on success
   */
  static bool DownloadMapSync(
      const AvailableMapEntry &entry,
      const std::filesystem::path &targetDir,
      HttpClient &httpClient,
      MapManagerRef mapManager,
      const std::function<void(uint64_t, uint64_t)> &progress = {});

  /**
   * Download a map to the specified directory.
   *
   * @param entry      the map entry to download
   * @param targetDir  local directory to download into
   * @param httpClient HTTP client for the download
   * @param progress   optional callback invoked with bytes downloaded and total bytes
   * @return future that resolves to true on success
   */
  CancelableFuture<bool> DownloadMap(
      const AvailableMapEntry &entry,
      const std::filesystem::path &targetDir,
      HttpClient &httpClient,
      const std::function<void(uint64_t, uint64_t)> &progress = {});

  /**
   * Cancel a running download by target directory.
   *
   * @param targetDir the directory of the download to cancel
   */
  void CancelDownload(const std::filesystem::path &targetDir);

  /**
   * Get the list of active download job states.
   * Thread-safe.
   */
  std::vector<DownloadJobState> GetDownloads() const;

  /**
   * Return the ordered list of map database file names (mandatory + optional).
   * Useful for Java-side download orchestration.
   */
  static std::vector<std::string> MapFiles();

  /**
   * Prepare a target directory for a map download: create the directory,
   * remove any files left over from a previous attempt, and write
   * metadata.json based on the available map entry.
   *
   * This is the non-HTTP part of DownloadMapInternal, intended for the
   * JNI download path where HTTP is handled in Java.
   *
   * @param entry     the map entry to download
   * @param targetDir local directory to prepare
   * @return true on success
   */
  static bool PrepareMapDirectory(const AvailableMapEntry &entry,
                                  const std::filesystem::path &targetDir);

  /**
   * Register a completed map directory with the given map manager.
   *
   * @param targetDir  directory containing the downloaded map
   * @param mapManager map manager to register with
   * @return true on success
   */
  static bool RegisterMapDirectory(const std::filesystem::path &targetDir,
                                   MapManagerRef mapManager);

private:
  MapManagerRef mapManager;
  SettingsRef settings;

  mutable std::mutex jobsMutex;
  std::vector<DownloadJobState> jobs;

  /**
   * Internal download implementation.
   */
  static bool DownloadMapInternal(std::vector<DownloadJobState> &jobs,
                                  std::mutex &jobsMutex,
                                  MapManagerRef mapManager,
                                  const AvailableMapEntry &entry,
                                  const std::filesystem::path &targetDir,
                                  HttpClient &httpClient,
                                  Breaker &breaker,
                                  const std::function<void(uint64_t, uint64_t)> &progress = {});
};

/**
 * \ingroup ClientAPI
 */
using MapDownloadServiceRef = std::shared_ptr<MapDownloadService>;

}

#endif /* OSMSCOUT_CLIENT_MAPDOWNLOADSERVICE_H */
