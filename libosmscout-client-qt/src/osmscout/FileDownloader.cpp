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

  downloadLastReadTime.start();
}

FileDownloader::~FileDownloader()
{
  if (reply) {
    reply->deleteLater();
  }
}

void FileDownloader::startDownload()
{
  // start download
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    OSMScoutQt::GetInstance().GetUserAgent());
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

  if (downloaded > 0) {
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

  timeoutTimerId=startTimer(1000); // used to check for timeouts
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

  if (reply){
    reply->deleteLater();
    reply = nullptr;
  }

  isOk = false;
  emit error(err, false);
}

void FileDownloader::onNetworkReadyRead()
{
  downloadLastReadTime.restart();

  assert(reply);

  QByteArray chunk = reply->readAll();
  downloaded += chunk.size();

  emit downloadedBytes(downloaded);

  file.write(chunk);
  emit writtenBytes(downloaded);
}

uint64_t FileDownloader::getBytesDownloaded() const
{
  return downloaded;
}

void FileDownloader::onDownloaded()
{
  if (!reply) {
    return; // happens on error, after error cleanup and initiating retry
  }

  if (reply->error() != QNetworkReply::NoError) {
    return;
  }

  onNetworkReadyRead(); // update all data if needed

  if (reply) {
    reply->deleteLater();
  }
  reply = nullptr;

  onFinished();
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

    reply->deleteLater();
    reply = nullptr;

    QTimer::singleShot(downloadRetrySleepTime * 1e3,
                       this, SLOT(startDownload()));

    downloadRetries++;
    downloaded = downloaded;
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

  if (downloadLastReadTime.elapsed()*1e-3 > downloadTimeout) {
    if (restartDownload()){
      emit error("Timeout", true);
      return;
    }
    onError("Timeout");
  }
}

}
