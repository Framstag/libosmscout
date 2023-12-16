/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Lukáš Karas

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

#include <QThread>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) /* For compatibility with QT 5.6 */
#include <QRandomGenerator>
#endif

#include <osmscoutclient/OnlineTileProvider.h>

#include <osmscoutclientqt/OsmTileDownloader.h>
#include <osmscoutclientqt/OSMScoutQt.h>

namespace osmscout {

OsmTileDownloader::OsmTileDownloader(QString diskCacheDir,
                                     const OnlineTileProvider &provider):
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0) /* For compatibility with QT 5.6 */
  serverNumber(qrand()),
#else
  serverNumber(QRandomGenerator::global()->generate()),
#endif
  tileProvider(provider)
{
  /** http://wiki.openstreetmap.org/wiki/Tile_usage_policy
   *
   * - Valid User-Agent identifying application. Faking another app's User-Agent WILL get you blocked.
   * - If known, a valid HTTP Referer.
   * - DO NOT send no-cache headers. ("Cache-Control: no-cache", "Pragma: no-cache" etc.)
   * - Cache Tile downloads locally according to HTTP Expiry Header, alternatively a minimum of 7 days.
   * - Maximum of 2 download threads. (Unmodified web browsers' download thread limits are acceptable.)
   */


  diskCache.setCacheDirectory(diskCacheDir);
  webCtrl.setCache(&diskCache);
}

OsmTileDownloader::~OsmTileDownloader() {
}

void OsmTileDownloader::onlineTileProviderChanged(const OnlineTileProvider &provider)
{
  tileProvider=provider;
}

void OsmTileDownloader::download(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
  if (!tileProvider.isValid()){
    emit failed(zoomLevel, x, y, false);
    return;
  }
  if ((int)zoomLevel > tileProvider.getMaximumZoomLevel()){
    emit failed(zoomLevel, x, y, true);
    return;
  }

  auto servers = tileProvider.getServers();
  if (servers.empty()){
    emit failed(zoomLevel, x, y, false);
    return;
  }
  QString server = QString::fromStdString(servers[serverNumber % servers.size()]);

  QUrl tileUrl(server.arg(zoomLevel).arg(x).arg(y));
  osmscout::log.Debug() << "Download tile " << tileUrl.toString().toStdString();

  TileCacheKey key = {zoomLevel, x, y};

  QNetworkRequest request(tileUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader, OSMScoutQt::GetInstance().GetUserAgent());

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0) /* For compatibility with QT 5.6 */
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
  //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

  QNetworkReply *reply = webCtrl.get(request);
  connect(reply, &QNetworkReply::finished, [key, this, reply](){ this->fileDownloaded(key, reply); });
}

void OsmTileDownloader::fileDownloaded(const TileCacheKey &key, QNetworkReply *reply)
{
  QUrl url = reply->url();

  if (reply->error() != QNetworkReply::NoError){
    osmscout::log.Warn() << "Downloading " << url.toString().toStdString() << " failed with " << reply->errorString().toStdString();
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0) /* For compatibility with QT 5.6 */
    serverNumber = qrand(); // try another server for future requests
#else
    serverNumber = QRandomGenerator::global()->generate(); // try another server for future requests
#endif
    emit failed(key.zoomLevel, key.xtile, key.ytile, false);
  }else{
    QByteArray downloadedData = reply->readAll();

    QImage image;
    if (image.loadFromData(downloadedData, Q_NULLPTR)){
      osmscout::log.Debug() << "Downloaded tile " << url.toString().toStdString();

      emit downloaded(key.zoomLevel, key.xtile, key.ytile, image, downloadedData);
    }else{
      osmscout::log.Warn() << "Failed to load image data from " << url.toString().toStdString();
      emit failed(key.zoomLevel, key.xtile, key.ytile, false);
    }
  }

  reply->deleteLater();
}
}
