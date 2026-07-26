#ifndef OSMSCOUT_CLIENT_HTTPCLIENT_H
#define OSMSCOUT_CLIENT_HTTPCLIENT_H

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

#include <osmscoutclient/ClientImportExport.h>

#include <filesystem>
#include <functional>
#include <string>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Callback for download progress reporting.
 *
 * @param bytesDownloaded number of bytes transferred so far
 * @param totalBytes      total expected size (0 if unknown)
 * @return true to continue, false to cancel
 */
using ProgressCallback = std::function<bool(uint64_t bytesDownloaded, uint64_t totalBytes)>;

/**
 * \ingroup ClientAPI
 *
 * Abstract interface for HTTP operations.
 * Implementations wrap platform-specific or library-specific HTTP clients
 * (e.g., Qt QNetworkAccessManager, Java java.net.http.HttpClient via JNI).
 *
 * This keeps libosmscout-client free of any HTTP library dependency.
 */
class OSMSCOUT_CLIENT_API HttpClient
{
public:
  virtual ~HttpClient() = default;

  /**
   * Perform an HTTP GET request and return the response body as a string.
   *
   * @param url  the URL to fetch
   * @return response body on success, empty string on error
   */
  virtual std::string Fetch(const std::string &url) = 0;

  /**
   * Download a file from a URL to a local path.
   *
   * Progress is reported via the callback. The implementation SHOULD
   * call the callback periodically with the number of bytes transferred.
   * If the callback returns false, the download SHOULD be cancelled.
   *
   * @param url      the URL to download
   * @param dest     local filesystem path to write to
   * @param progress optional progress callback
   * @return true on success, false on error or cancellation
   */
  virtual bool Download(const std::string &url,
                        const std::filesystem::path &dest,
                        ProgressCallback progress = nullptr) = 0;
};

}

#endif /* OSMSCOUT_CLIENT_HTTPCLIENT_H */
