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

#include <osmscout/FileDownloader.h>
#include <osmscout/Settings.h>
#include <osmscout/OSMScoutQt.h>

#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QDebug>

#include <algorithm>
#include <iostream> // for a rarely expected error message

namespace osmscout {

FileDownloader::FileDownloader(QNetworkAccessManager *manager,
                               QString urlStr, QString path,
                               const Type mode,
                               QObject *parent):
  QObject(parent),
  manager(manager),
  url(urlStr),
  path(path)
{
  if (!url.isValid()) {
    isOk = false;
    return;
  }

  // check path and open file
  QFileInfo finfo(path);
  QDir dir;
  if ( !dir.mkpath(finfo.dir().absolutePath()) ) {
    isOk = false;
    return;
  }

  file.setFileName(path + ".download");
  if (!file.open(QIODevice::WriteOnly)) {
    isOk = false;
    return;
  }

  connect(&file, &QFile::bytesWritten,
          this, &FileDownloader::onBytesWritten);

  // start data processor if requested
  QString command;
  QStringList arguments;
  if (mode == BZ2) {
    command = "bunzip2";
    arguments << "-c";
  } else if (mode == Plain) {
    // nothing to do
  } else {
    std::cerr << "FileDownloader: unknown mode: " << mode << std::endl;
    isOk = false;
    return;
  }

  if (!command.isEmpty()) {
    pipeToProcess = true;
    process = new QProcess(this);

    connect( process, &QProcess::started,
             this, &FileDownloader::onProcessStarted );

    connect( process, SIGNAL(finished(int)),
             this, SLOT(onProcessStopped(int)) );

    connect( process, &QProcess::stateChanged,
             this, &FileDownloader::onProcessStateChanged);

    connect( process, &QProcess::readyReadStandardOutput,
             this, &FileDownloader::onProcessRead);

    connect( process, &QProcess::readyReadStandardError,
             this, &FileDownloader::onProcessReadError);

    connect( process, &QProcess::bytesWritten,
             this, &FileDownloader::onBytesWritten);

    process->start(command, arguments);
  }

  downloadLastReadTime.start();
  downloadThrottleTimeStart.start();
}

FileDownloader::~FileDownloader()
{
  if (reply) {
    reply->deleteLater();
  }
  if (process) {
    process->deleteLater();
  }
}

void FileDownloader::startDownload()
{
  // start download
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    OSMScoutQt::GetInstance().GetUserAgent());
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

  if (downloaded > 0)
    {
      // TODO: Range header don't have to be supported by server, we should handle such case
      QByteArray range_header = "bytes=" + QByteArray::number((qulonglong)downloaded) + "-";
      request.setRawHeader("Range",range_header);
    }

  reply = manager->get(request);
  reply->setReadBufferSize(bufferNetwork);

  connect(reply, &QNetworkReply::readyRead,
          this, &FileDownloader::onNetworkReadyRead);
  connect(reply, &QNetworkReply::finished,
          this, &FileDownloader::onDownloaded);
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(onNetworkError(QNetworkReply::NetworkError)));

  downloadLastReadTime.restart();
  downloadThrottleTimeStart.restart();
  downloadThrottleBytes = 0;

  timeoutTimerId=startTimer(1000); // used to check for timeouts and in throttling network speed
}

void FileDownloader::onFinished()
{
  file.close();

  { // delete the file if it exists already to update it
    // with a new copy
    QFile ftmp(path);
    ftmp.remove();
  }

  file.rename(path);

  if (process) {
    process->deleteLater();
    process = nullptr;
  }

  if (reply) {
    reply->deleteLater();
    reply = nullptr;
  }

  emit finished(path);
}

void FileDownloader::onError(const QString &err)
{
  file.close();
  file.remove();

  if (process)
    {
      process->deleteLater();
      process = nullptr;
    }

  if (reply)
    {
      reply->deleteLater();
      reply = nullptr;
    }

  isOk = false;
  emit error(err, false);
}

void FileDownloader::onNetworkReadyRead()
{
  downloadLastReadTime.restart();

  if (!reply ||
      (pipeToProcess && !processStarted) ) {
    // too early, haven't started yet
    return;
  }

  double speed = downloadThrottleBytes /
      (downloadThrottleTimeStart.elapsed() * 1e-3) / 1024.0;

  //  qDebug() << "Buffers: "
  //           << m_reply->bytesAvailable() << " [network] / "
  //           << m_reply->readBufferSize() << " [network max] / "
  //           << (m_pipe_to_process ? m_process->bytesToWrite() : -1)
  //           << " [process] / " << m_file.bytesToWrite() << " [file]; "
  //           << "speed [kb/s]: " << speed
  //           << " / clear: " << m_clear_all_caches;

  // check if the network has to be throttled due to excessive
  // non-writen buffers. check is skipped on the last read called
  // with m_clear_all_caches
  if (!clearAllCaches) {
    /// It seems that sometimes Qt heavily overshoots the requested network buffer size. In
    /// particular it has been usual for the first download from start of the server. On the second try,
    /// it's usually OK (seen on SFOS 2.0 series)
    if ( reply->bytesAvailable() > bufferNetworkMaxFactorBeforeCancel*bufferNetwork ) {
      restartDownload(true);
      return;
    }

    if ( (pipeToProcess && process->bytesToWrite() > bufferSizeIO) ||
         (file.bytesToWrite() > bufferSizeIO) ) {
      pauseNetworkIo = true;
      return;
    }

    // check if requested speed has been exceeded
    if (downloadThrottleMaxSpeed > 0) {
      if (speed > downloadThrottleMaxSpeed) {
        //              qDebug() << "Going too fast: " << speed;
        pauseNetworkIo = true;
        return;
      }
    }
  }

  pauseNetworkIo = false;

  QByteArray data_current;
  if (clearAllCaches) {
    data_current = reply->readAll();
  } else {
    data_current = reply->read( std::min(cacheSizeBeforeSwap, bufferNetwork) );
  }

  cacheCurrent.append(data_current);
  downloadedGui += data_current.size();
  downloadThrottleBytes += data_current.size();

  emit downloadedBytes(downloadedGui);

  // check if caches are full or whether they have to be
  // filled before writing to file/process
  if (!clearAllCaches && cacheCurrent.size() < cacheSizeBeforeSwap) {
    return;
  }

  QByteArray data(cacheSafe);
  if (clearAllCaches) {
    data.append(cacheCurrent);
    cacheCurrent.clear();
    cacheSafe.clear();
  } else {
    cacheSafe = cacheCurrent;
    cacheCurrent.clear();
  }

  downloaded += data.size();

  if (pipeToProcess) {
    process->write(data);
  } else {
    file.write(data);
    emit writtenBytes(downloaded);
  }
}

uint64_t FileDownloader::getBytesDownloaded() const
{
  return downloadedGui;
}

void FileDownloader::onDownloaded()
{
  if (!reply) {
    return; // happens on error, after error cleanup and initiating retry
  }

  if (reply->error() != QNetworkReply::NoError) {
    return;
  }

  if (pipeToProcess && !processStarted) {
    return;
  }

  clearAllCaches = true;
  onNetworkReadyRead(); // update all data if needed

  if (pipeToProcess && process) {
    process->closeWriteChannel();
  }

  if (reply) {
    reply->deleteLater();
  }
  reply = nullptr;

  if (!pipeToProcess) {
    onFinished();
  }
}

bool FileDownloader::restartDownload(bool force)
{
  killTimer(timeoutTimerId);
  //  qDebug() << QTime::currentTime() << " / Restart called: "
  //           << url << " " << m_download_retries << " " << m_download_last_read_time.elapsed();

  // check if we should retry before cancelling all with an error
  // this check is performed only if we managed to get some data
  if (downloadedLastError != downloaded) {
    downloadRetries = 0;
  }

  if (reply &&
      downloadRetries < maxDownloadRetries &&
      (downloaded > 0 || force) ) {
    cacheSafe.clear();
    cacheCurrent.clear();
    reply->deleteLater();
    reply = nullptr;

    QTimer::singleShot(downloadRetrySleepTime * 1e3,
                       this, SLOT(startDownload()));

    downloadRetries++;
    downloadedGui = downloaded;
    downloadedLastError = downloaded;
    downloadLastReadTime.restart();

    return true;
  }

  return false;
}

void FileDownloader::onNetworkError(QNetworkReply::NetworkError /*code*/)
{
  if (restartDownload()) {
    emit error(reply? reply->errorString(): "", true);
    return;
  }

  if (reply) {
    onError(reply->errorString());
  }
}

void FileDownloader::timerEvent(QTimerEvent * /*event*/)
{
  if (pauseNetworkIo) {
    onNetworkReadyRead();
  }

  if (downloadLastReadTime.elapsed()*1e-3 > downloadTimeout) {
    if (restartDownload()){
      emit error("Timeout", true);
      return;
    };
    onError("Timeout");
  }
}

void FileDownloader::onBytesWritten(qint64)
{
  if (pauseNetworkIo) {
    onNetworkReadyRead();
  }
}

void FileDownloader::onProcessStarted()
{
  processStarted = true;
  onNetworkReadyRead(); // pipe all data in that has been collected already
}

void FileDownloader::onProcessRead()
{
  downloadLastReadTime.restart();

  if (!process) {
    return;
  }

  QByteArray data = process->readAllStandardOutput();
  file.write(data);
  written += data.size();
  emit writtenBytes(written);
}

void FileDownloader::onProcessStopped(int exitCode)
{
  if (exitCode != 0){
    QString err = osmscout::FileDownloader::tr("Error in processing downloaded data");
    isOk = false;
    emit error(err, false);
    return;
  }

  if (!process){
    return;
  }

  onProcessRead();
  onFinished();
}

void FileDownloader::onProcessReadError()
{
  if (!process){
    // should not happen
    return;
  }

  QByteArray data = process->readAllStandardError();
  if (data.size() > 0){
    onError("Error in processing downloaded data");
  }
}

void FileDownloader::onProcessStateChanged(QProcess::ProcessState state)
{
  if ( !processStarted && state == QProcess::NotRunning ) {
    QString err = osmscout::FileDownloader::tr("Error in processing downloaded data: could not start the program") + " " + process->program();
    onError(err);
  }
}
}
