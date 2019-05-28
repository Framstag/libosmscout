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

template<typename T>
int64_t AsMillis(const std::chrono::duration<T> &duration){
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

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
    qWarning() << "Url is not valid:" << url;
    return;
  }

  // check path and open file
  QFileInfo finfo(path);
  QDir dir;
  if (!dir.mkpath(finfo.dir().absolutePath())) {
    isOk = false;
    qWarning() << "Cannot create directory:" << finfo.dir().absolutePath();
    return;
  }

  file.setFileName(path + ".download");
  if (!file.open(QIODevice::WriteOnly)) {
    isOk = false;
    qWarning() << "Cannot open file:" << file.fileName();
    return;
  }

  timeoutTimer.setSingleShot(true);
  timeoutTimer.setInterval(AsMillis(FileDownloaderConfig::DownloadTimeout));
  connect(&timeoutTimer, &QTimer::timeout, this, &FileDownloader::onTimeout);

  backOff.restartTimer.setSingleShot(true);
  connect(&backOff.restartTimer, &QTimer::timeout, this, &FileDownloader::startDownload);
}

FileDownloader::~FileDownloader()
{
  if (reply) {
    reply->deleteLater();
  }
  if (file.exists() && !finishedSuccessfully) {
    file.close();
    file.remove();
  }
}

void FileDownloader::startDownload()
{
  // start download
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    OSMScoutQt::GetInstance().GetUserAgent());
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

  qDebug() << "Start downloading" << url << "to" << file.fileName();
  if (downloaded > 0) {
    // TODO: Range header don't have to be supported by server, we should handle such case
    QByteArray range_header = "bytes=" + QByteArray::number((qulonglong)downloaded) + "-";
    qDebug() << "Request from byte" << downloaded;
    request.setRawHeader("Range",range_header);
  }

  reply = manager->get(request);
  reply->setReadBufferSize(FileDownloaderConfig::BufferNetwork);

  connect(reply, &QNetworkReply::readyRead,
          this, &FileDownloader::onNetworkReadyRead);
  connect(reply, &QNetworkReply::finished,
          this, &FileDownloader::onDownloaded);
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(onNetworkError(QNetworkReply::NetworkError)));

  timeoutTimer.start();
}

void FileDownloader::onFinished()
{
  file.close();

  { // delete the file if it exists already to update it
    // with a new copy
    QFile ftmp(path);
    ftmp.remove();
  }

  qDebug() << "Downloaded" << downloaded << "bytes";
  finishedSuccessfully = file.rename(path);

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
  timeoutTimer.start();
  backOff.recover();

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

void FileDownloader::BackOff::recover()
{
  if (downloadRetries > 0) {
    restartTimer.stop();
    backOffTime = FileDownloaderConfig::BackOffInitial;
    downloadRetries = 0;
  }
}

bool FileDownloader::BackOff::scheduleRestart()
{
  if (FileDownloaderConfig::MaxDownloadRetries >= 0 && downloadRetries >= FileDownloaderConfig::MaxDownloadRetries) {
    return false;
  }

  qDebug() << "Back off" << backOffTime.count() << "s";
  restartTimer.setInterval(AsMillis(backOffTime));
  restartTimer.start();
  backOffTime=std::min(backOffTime*2, FileDownloaderConfig::BackOffMax);

  downloadRetries++;
  return true;
}

bool FileDownloader::restartDownload()
{
  timeoutTimer.stop();
  qDebug() << QTime::currentTime() << "Restart called:"
           << url << "retries:" <<  backOff.downloadRetries;

  if (reply){
    reply->deleteLater();
    reply = nullptr;
  }
  return backOff.scheduleRestart();
}

void FileDownloader::onNetworkError(QNetworkReply::NetworkError /*code*/)
{
  QString errorStr = reply ? reply->errorString(): "";
  if (restartDownload()) {
    emit error(errorStr, true);
    return;
  }

  if (reply) {
    onError(reply->errorString());
  }
}

void FileDownloader::onTimeout()
{
  if (restartDownload()){
    emit error("Timeout", true);
  }else {
    onError("Timeout");
  }
}

}
