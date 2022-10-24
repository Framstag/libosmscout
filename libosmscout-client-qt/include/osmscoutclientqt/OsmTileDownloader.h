#ifndef OSMSCOUT_CLIENT_QT_OSMTILEDOWNLOADER_H
#define OSMSCOUT_CLIENT_QT_OSMTILEDOWNLOADER_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QImage>
#include <QNetworkDiskCache>

#include <osmscoutclientqt/TileCache.h>
#include <osmscoutclientqt/OnlineTileProvider.h>
#include <osmscoutclientqt/Settings.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OsmTileDownloader : public QObject
{
  Q_OBJECT

public:
  OsmTileDownloader(QString diskCacheDir,
                    const OnlineTileProvider &provider);
  ~OsmTileDownloader() override;

public slots:
  void download(uint32_t zoomLevel, uint32_t x, uint32_t y);
  void onlineTileProviderChanged(const OnlineTileProvider &provider);

signals:
  void downloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void failed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);

private slots:
  void fileDownloaded(const TileCacheKey &key, QNetworkReply *reply);

private:
  quint32                   serverNumber;
  QNetworkAccessManager     webCtrl;
  QNetworkDiskCache         diskCache;
  OnlineTileProvider        tileProvider;

};

}

#endif /* OSMSCOUT_CLIENT_QT_OSMTILEDOWNLOADER_H */
