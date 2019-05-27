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
/// as needed. If specified that the format is BZ2, the downloader pipes
/// the internet stream through a process running bunzip2.
class OSMSCOUT_CLIENT_QT_API FileDownloader : public QObject
{
  Q_OBJECT

public:
  enum Type { Plain=0, BZ2=1 };

public:
  explicit FileDownloader(QNetworkAccessManager *manager,
                          QString url,
                          QString path,
                          const Type mode = Plain,
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

  void onProcessStarted();
  void onProcessRead();
  void onProcessStopped(int exitCode); ///< Called on error while starting or when process has stopped
  void onBytesWritten(qint64);

protected:
  void onProcessStateChanged(QProcess::ProcessState newState); ///< Called when state of the process has changed
  void onProcessReadError();

  void onFinished();
  void onError(const QString &err);

  bool restartDownload(bool force = false); ///< Restart download if download retries are not used up

  virtual void timerEvent(QTimerEvent *event);

protected:
  QNetworkAccessManager *manager;
  QUrl url;
  QString path;

  QNetworkReply *reply{nullptr};

  QProcess *process{nullptr};
  bool processStarted{false};

  QFile file;

  bool pipeToProcess{false};
  bool isOk{true};

  QByteArray cacheSafe;
  QByteArray cacheCurrent;
  bool clearAllCaches{false};
  bool pauseNetworkIo{false};

  uint64_t downloaded{0};
  uint64_t written{0};
  uint64_t downloadedGui{0};

  uint64_t downloadThrottleBytes{0};
  QTime downloadThrottleTimeStart;
  double downloadThrottleMaxSpeed{0};

  uint64_t downloadedLastError{0};
  size_t downloadRetries{0};
  QTime downloadLastReadTime;
  int timeoutTimerId{-1};

  const size_t maxDownloadRetries{5};          ///< Maximal number of download retries before cancelling download
  const double downloadRetrySleepTime{30.0};  ///< Time between retries in seconds

  const qint64 cacheSizeBeforeSwap{1024*1024*1}; ///< Size at which cache is promoted from network to file/process
  const qint64 bufferSizeIO{1024*1024*3};         ///< Size of the buffers that should not be significantly exceeded
  const qint64 bufferNetwork{1024*1024*1};         ///< Size of network ring buffer

  /// \brief Factor determining whether cancel download when network buffer is too large
  const qint64 bufferNetworkMaxFactorBeforeCancel{10};

  const int downloadTimeout{60};                ///< Download timeout in seconds
};

}

#endif // OSMSCOUT_CLIENT_QT_FILEDOWNLOADER_H
