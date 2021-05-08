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
  timeoutTimer.setInterval(AsMillis(FileDownloaderConfig::DownloadReadTimeout));
  connect(&timeoutTimer, &QTimer::timeout, this, &FileDownloader::onTimeout);

  backOff.restartTimer.setSingleShot(true);
  connect(&backOff.restartTimer, &QTimer::timeout, this, &FileDownloader::startDownload);
}

FileDownloader::~FileDownloader()
{
  if (reply != nullptr) {
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

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0) /* For compatibility with QT 5.6 */
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif

  qDebug() << "Start downloading" << url << "to" << file.fileName();
  if (downloaded > 0) {
    // TODO: Range header don't have to be supported by server, we should handle such case
    QByteArray range_header = "bytes=" + QByteArray::number((qulonglong)downloaded) + "-";
    qDebug() << "Request from byte" << downloaded;
    request.setRawHeader("Range",range_header);

    /**
     * Default value for "Accept-Encoding" in Qt is "gzip, deflate"
     * and Qt code do the decompressing for us (when server reply with "Content-Encoding: gzip").
     * But with explicit byte range (content not from the beginning) decompressing is not possible
     * (gzip header is not valid) and Qt fails with NetworkError::ProtocolFailure
     *
     * For that reason we have to specify that only accepted encoding is identity.
     */
    request.setRawHeader("Accept-Encoding", "identity");
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

  if (reply != nullptr) {
    reply->deleteLater();
    reply = nullptr;
  }

  emit finished(path);
}

void FileDownloader::onError(const QString &err)
{
  file.close();
  file.remove();

  if (reply != nullptr){
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

  // TODO: is there any case when writeRes != chunk.size() and it is not error?
  qint64 writeRes = file.write(chunk);
  if (writeRes < 0 || writeRes != chunk.size()){
    qWarning() << "Writing to file return with:" << writeRes << ", chunk size:" << chunk.size() << ", error:" << file.errorString();
    QString error = file.errorString();
    if (error.isEmpty()){
      error = "Writing to file failed";
    }
    onError(error);
  }else {
    emit writtenBytes(downloaded);
  }
}

uint64_t FileDownloader::getBytesDownloaded() const
{
  return downloaded;
}

void FileDownloader::onDownloaded()
{
  if (reply == nullptr) {
    return; // happens on error, after error cleanup and initiating retry
  }

  if (reply->error() != QNetworkReply::NoError) {
    return;
  }

  onNetworkReadyRead(); // update all data if needed

  if (reply != nullptr) {
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

  if (reply != nullptr){
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    reply->deleteLater();
    reply = nullptr;

    if (statusCode.isValid() && statusCode.toInt() >= 400 && statusCode.toInt() < 500){
      return false; // client error is not recoverable (http 400..499)
    }
  }
  return backOff.scheduleRestart();
}

void FileDownloader::onNetworkError(QNetworkReply::NetworkError code)
{
  QString errorStr = reply != nullptr ? reply->errorString(): "";
  qDebug() << "Error " << code << "/" << errorStr;

  QVariant statusCode = reply != nullptr ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) : QVariant();
  if (statusCode.isValid()) {
    QString reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    qWarning() << "Server status code" << statusCode.toInt() << ":" << reason;
  }

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
    emit error("Read timeout", true);
  }else {
    onError("Read timeout");
  }
}

DownloadJob::DownloadJob(QNetworkAccessManager *webCtrl, QDir target, bool replaceExisting):
  webCtrl{webCtrl}, target{target}, replaceExisting{replaceExisting}
{
}

DownloadJob::~DownloadJob()
{
  clearJobs();
}

void DownloadJob::clearJobs()
{
  for (auto* job:jobs){
    delete job;
  }
  jobs.clear();
}

void DownloadJob::start(const QString &serverBasePath, const QStringList &fileNames)
{
  for (const auto& fileName:fileNames){
    auto *job=new FileDownloader(webCtrl, serverBasePath+"/"+fileName, target.filePath(fileName));
    connect(job, &FileDownloader::finished, this, &MapDownloadJob::onJobFinished);
    connect(job, &FileDownloader::error, this, &MapDownloadJob::onJobFailed);
    connect(job, &FileDownloader::writtenBytes, this, &MapDownloadJob::downloadProgress);
    connect(job, &FileDownloader::writtenBytes, this, &MapDownloadJob::onDownloadProgress);
    jobs << job;
  }
  started=true;
  downloadNextFile();
}

void DownloadJob::cancel()
{
  if (!done){
    canceledByUser=true;
    onJobFailed("Canceled by user", false);
  }
}

void DownloadJob::onDownloadProgress(uint64_t)
{
  // reset error message
  error = "";
}

void DownloadJob::onJobFailed(QString errorMessage, bool recoverable){
  osmscout::log.Warn() << "Download failed with the error: "
                       << errorMessage.toStdString() << " "
                       << (recoverable? "(recoverable)": "(not recoverable)");

  if (recoverable){
    error = errorMessage;
    emit downloadProgress();
  }else{
    done = true;
    error = errorMessage;
    clearJobs();
    if (canceledByUser) {
      emit canceled();
    }else{
      emit failed(errorMessage);
    }
  }
}

void DownloadJob::onJobFinished([[maybe_unused]] QString path)
{
  if (!jobs.isEmpty()) {
    FileDownloader* job = jobs.first();
    jobs.pop_front();
    assert(job->getFilePath()==path);
    downloadedBytes += job->getBytesDownloaded();
    job->deleteLater();
  }

  downloadNextFile();
}

void DownloadJob::downloadNextFile()
{
  if (!jobs.isEmpty()) {
    jobs.first()->startDownload();
    emit downloadProgress();
  } else {
    done = true;
    successful = true;
    emit finished();
  }
}

double DownloadJob::getProgress()
{
  double expected=expectedSize();
  uint64_t downloaded=downloadedBytes;
  for (auto *job:jobs){
    downloaded+=job->getBytesDownloaded();
  }
  if (expected==0.0)
    return 0;
  return (double)downloaded/expected;
}

QString DownloadJob::getDownloadingFile()
{
  if (!jobs.isEmpty())
    return jobs.first()->getFileName();
  return "";
}


}
