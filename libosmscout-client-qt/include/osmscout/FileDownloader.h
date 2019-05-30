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
#include <QTimer>

#include <osmscout/ClientQtImportExport.h>

#include <chrono>

namespace osmscout {

namespace FileDownloaderConfig {

static constexpr uint64_t BufferNetwork{1024*1024*1}; ///< Size of network ring buffer
static constexpr std::chrono::seconds DownloadTimeout{60}; ///< Download timeout in seconds
static constexpr std::chrono::seconds BackOffInitial{1}; ///< Initial back-off time
static constexpr std::chrono::seconds BackOffMax{300}; ///< Maximum back-off time
static constexpr int MaxDownloadRetries{-1}; ///< Maximal number of download retries before cancelling download
}

/// \brief Downloads a file specified by URL
///
/// Downloads a file as specified by URL and stores in a given path.
/// If the required directories do not exist, creates all parent directories
/// as needed.
class OSMSCOUT_CLIENT_QT_API FileDownloader : public QObject
{
  Q_OBJECT

private:
  struct BackOff {
    int downloadRetries{0};

    std::chrono::seconds backOffTime{FileDownloaderConfig::BackOffInitial};
    QTimer restartTimer;

    bool scheduleRestart();
    void recover();
  };

  BackOff backOff;

  QNetworkAccessManager *manager;
  QUrl url;
  QString path;

  QNetworkReply *reply{nullptr};

  QFile file;

  bool isOk{true};
  bool finishedSuccessfully{false};

  uint64_t downloaded{0};

  QTimer timeoutTimer;

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
  void onTimeout();

protected:
  void onFinished();
  void onError(const QString &err);

  bool restartDownload(); ///< Restart download if download retries are not used up
};

}

#endif // OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_H
