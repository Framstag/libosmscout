#ifndef OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_H
#define OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Rinigus

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
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>
#include <QProcess>
#include <QByteArray>
#include <QTime>
#include <QFileInfo>

#include <osmscout/ClientQtImportExport.h>

namespace osmscout {

/// \brief Downloads a file specified by URL
///
/// Downloads a file as specified by URL and stores in a given path.
/// If the required directories do not exist, creates all parent directories
/// as needed.
class OSMSCOUT_CLIENT_QT_API FileDownloader : public QObject
{
  Q_OBJECT


public:
  explicit FileDownloader(QNetworkAccessManager *manager,
                          QString url,
                          QString path,
                          QObject *parent = 0);
  ~FileDownloader();

  operator bool() const { return isOk; }
  QString getFileName() const { return QFileInfo(path).fileName(); }
  QString getFilePath() const { return path; }
  uint64_t getBytesDownloaded() const;

signals:
  void downloadedBytes(uint64_t sz);
  void writtenBytes(uint64_t sz);
  void finished(QString path);
  void error(QString error_text, bool recoverable);

public slots:
  void startDownload();

protected slots:
  void onNetworkReadyRead();
  void onDownloaded();
  void onNetworkError(QNetworkReply::NetworkError code);

protected:
  void onFinished();
  void onError(const QString &err);

  bool restartDownload(bool force = false); ///< Restart download if download retries are not used up

  virtual void timerEvent(QTimerEvent *event);

protected:
  QNetworkAccessManager *manager;
  QUrl url;
  QString path;

  QNetworkReply *reply{nullptr};

  QFile file;

  bool isOk{true};

  uint64_t downloaded{0};

  uint64_t downloadedLastError{0};
  size_t downloadRetries{0};
  QTime downloadLastReadTime;
  int timeoutTimerId{-1};

  const size_t maxDownloadRetries{5};          ///< Maximal number of download retries before cancelling download
  const double downloadRetrySleepTime{30.0};  ///< Time between retries in seconds

  const qint64 bufferNetwork{1024*1024*1};         ///< Size of network ring buffer

  const int downloadTimeout{60};                ///< Download timeout in seconds
};

}

#endif // OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_H
