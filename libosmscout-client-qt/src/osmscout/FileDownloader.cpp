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

#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QDebug>

#include <algorithm>
#include <iostream> // for a rarely expected error message

FileDownloader::FileDownloader(QNetworkAccessManager *manager,
                               QString url, QString path,
                               const Type mode,
                               QObject *parent):
  QObject(parent),
  m_manager(manager),
  m_url(url), m_path(path)
{
  if (!m_url.isValid())
    {
      m_isok = false;
      return;
    }

  // check path and open file
  QFileInfo finfo(m_path);
  QDir dir;
  if ( !dir.mkpath(finfo.dir().absolutePath()) )
    {
      m_isok = false;
      return;
    }

  m_file.setFileName(m_path + ".download");
  if (!m_file.open(QIODevice::WriteOnly))
    {
      m_isok = false;
      return;
    }

  connect( &m_file, &QFile::bytesWritten,
           this, &FileDownloader::onBytesWritten );

  // start data processor if requested
  QString command;
  QStringList arguments;
  if (mode == BZ2)
    {
      command = "bunzip2";
      arguments << "-c";
    }
  else if (mode == Plain)
    {
      // nothing to do
    }
  else
    {
      std::cerr << "FileDownloader: unknown mode: " << mode << std::endl;
      m_isok = false;
      return;
    }

  if (!command.isEmpty())
    {
      m_pipe_to_process = true;
      m_process = new QProcess(this);

      connect( m_process, &QProcess::started,
               this, &FileDownloader::onProcessStarted );

      connect( m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
               this, &FileDownloader::onProcessStopped );

      connect( m_process, &QProcess::stateChanged,
               this, &FileDownloader::onProcessStateChanged );

      connect( m_process, &QProcess::readyReadStandardOutput,
               this, &FileDownloader::onProcessRead );

      connect( m_process, &QProcess::readyReadStandardError,
               this, &FileDownloader::onProcessReadError );

      connect( m_process, &QProcess::bytesWritten,
               this, &FileDownloader::onBytesWritten );

      m_process->start(command, arguments);
    }

  m_download_last_read_time.start();
  m_download_throttle_time_start.start();

  // start download
  // startDownload();
  startTimer(1000); // used to check for timeouts and in throttling network speed
}

FileDownloader::~FileDownloader()
{
  if (m_reply) m_reply->deleteLater();
  if (m_process) m_process->deleteLater();
}

void FileDownloader::startDownload()
{
  // qDebug() << "Start or ReStart: " << m_url;

  // start download
  QNetworkRequest request(m_url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QString(OSMSCOUT_USER_AGENT).arg(OSMSCOUT_VERSION_STRING));

  if (m_downloaded > 0)
    {
      QByteArray range_header = "bytes=" + QByteArray::number((qulonglong)m_downloaded) + "-";
      request.setRawHeader("Range",range_header);
    }

  m_reply = m_manager->get(request);
  m_reply->setReadBufferSize(const_buffer_network);

  connect(m_reply, SIGNAL(readyRead()),
          this, SLOT(onNetworkReadyRead()));
  connect(m_reply, SIGNAL(finished()),
          this, SLOT(onDownloaded()));
  connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(onNetworkError(QNetworkReply::NetworkError)));

  m_download_last_read_time.restart();
  m_download_throttle_time_start.restart();
  m_download_throttle_bytes = 0;
}

void FileDownloader::onFinished()
{
  m_file.close();

  { // delete the file if it exists already to update it
    // with a new copy
    QFile ftmp(m_path);
    ftmp.remove();
  }

  m_file.rename(m_path);

  if (m_process)
    {
      m_process->deleteLater();
      m_process = nullptr;
    }

  if (m_reply)
    {
      m_reply->deleteLater();
      m_reply = nullptr;
    }

  emit finished(m_path);
}

void FileDownloader::onError(const QString &err)
{
  m_file.close();
  m_file.remove();

  if (m_process)
    {
      m_process->deleteLater();
      m_process = nullptr;
    }

  if (m_reply)
    {
      m_reply->deleteLater();
      m_reply = nullptr;
    }

  m_isok = false;
  emit error(err);
}

void FileDownloader::onNetworkReadyRead()
{
  m_download_last_read_time.restart();

  if (!m_reply ||
      (m_pipe_to_process && !m_process_started) ) // too early, haven't started yet
    return;

  double speed = m_download_throttle_bytes /
      (m_download_throttle_time_start.elapsed() * 1e-3) / 1024.0;

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
  if (!m_clear_all_caches)
    {
      /// It seems that sometimes Qt heavily overshoots the requested network buffer size. In
      /// particular it has been usual for the first download from start of the server. On the second try,
      /// it's usually OK (seen on SFOS 2.0 series)
      if ( m_reply->bytesAvailable() > const_buffer_network_max_factor_before_cancel*const_buffer_network )
        {
          restartDownload(true);
          return;
        }

      if ( (m_pipe_to_process && m_process->bytesToWrite() > const_buffer_size_io) ||
           (m_file.bytesToWrite() > const_buffer_size_io) )
        {
          m_pause_network_io = true;
          return;
        }

      // check if requested speed has been exceeded
      if (m_download_throttle_max_speed > 0)
        {
          if (speed > m_download_throttle_max_speed)
            {
              //              qDebug() << "Going too fast: " << speed;
              m_pause_network_io = true;
              return;
            }
        }
    }

  m_pause_network_io = false;

  QByteArray data_current;
  if (m_clear_all_caches) data_current = m_reply->readAll();
  else data_current = m_reply->read( std::min(const_cache_size_before_swap, const_buffer_network) );

  m_cache_current.append(data_current);
  m_downloaded_gui += data_current.size();
  m_download_throttle_bytes += data_current.size();

  emit downloadedBytes(m_downloaded_gui);

  // check if caches are full or whether they have to be
  // filled before writing to file/process
  if (!m_clear_all_caches && m_cache_current.size() < const_cache_size_before_swap)
    return;

  QByteArray data(m_cache_safe);
  if (m_clear_all_caches)
    {
      data.append(m_cache_current);
      m_cache_current.clear();
      m_cache_safe.clear();
    }
  else
    {
      m_cache_safe = m_cache_current;
      m_cache_current.clear();
    }

  m_downloaded += data.size();

  if (m_pipe_to_process)
    {
      m_process->write(data);
    }
  else
    {
      m_file.write(data);
      emit writtenBytes(m_downloaded);
    }
}

uint64_t FileDownloader::getBytesDownloaded() const
{
  return m_downloaded_gui;
}

void FileDownloader::onDownloaded()
{
  if (!m_reply) return; // happens on error, after error cleanup and initiating retry

  if (m_reply->error() != QNetworkReply::NoError)
    return;

  if (m_pipe_to_process && !m_process_started)
    return;

  m_clear_all_caches = true;
  onNetworkReadyRead(); // update all data if needed

  if (m_pipe_to_process && m_process)
    m_process->closeWriteChannel();

  if (m_reply) m_reply->deleteLater();
  m_reply = nullptr;

  if (!m_pipe_to_process) onFinished();
}

bool FileDownloader::restartDownload(bool force)
{
//  qDebug() << QTime::currentTime() << " / Restart called: "
//           << m_url << " " << m_download_retries << " " << m_download_last_read_time.elapsed();

  // check if we should retry before cancelling all with an error
  // this check is performed only if we managed to get some data
  if (m_downloaded_last_error != m_downloaded)
    m_download_retries = 0;

  if (m_reply &&
      m_download_retries < const_max_download_retries &&
      (m_downloaded > 0 || force) )
    {
      m_cache_safe.clear();
      m_cache_current.clear();
      m_reply->deleteLater();
      m_reply = nullptr;

      QTimer::singleShot(const_download_retry_sleep_time * 1e3,
                         this, SLOT(startDownload()));

      m_download_retries++;
      m_downloaded_gui = m_downloaded;
      m_downloaded_last_error = m_downloaded;
      m_download_last_read_time.restart();

      return true;
    }

  return false;
}

void FileDownloader::onNetworkError(QNetworkReply::NetworkError /*code*/)
{
  if (restartDownload()) return;

  if (m_reply)
    {
      QString err = tr("Failed to download") + "<br>" + m_path +
          "<br><br>" +tr("Error code: %1").arg(QString::number(m_reply->error())) + "<br><blockquote><small>" +
          m_reply->errorString() +
          "</small></blockquote>";
      onError(err);
    }
}

void FileDownloader::timerEvent(QTimerEvent * /*event*/)
{
  if (m_pause_network_io)
    onNetworkReadyRead();

  if (m_download_last_read_time.elapsed()*1e-3 > const_download_timeout)
    {
      if (restartDownload()) return;

      QString err = tr("Failed to download") + "<br>" + m_path +
          "<br><br>" +tr("Timeout");

      onError(err);
    }
}

void FileDownloader::onBytesWritten(qint64)
{
  if (m_pause_network_io)
    onNetworkReadyRead();
}

void FileDownloader::onProcessStarted()
{
  m_process_started = true;
  onNetworkReadyRead(); // pipe all data in that has been collected already
}

void FileDownloader::onProcessRead()
{
  m_download_last_read_time.restart();

  if (!m_process) return;

  QByteArray data = m_process->readAllStandardOutput();
  m_file.write(data);
  m_written += data.size();
  emit writtenBytes(m_written);
}

void FileDownloader::onProcessStopped(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
  if (exitCode != 0)
    {
      QString err = tr("Error in processing downloaded data");
      m_isok = false;
      emit error(err);
      return;
    }

  if (!m_process) return;

  onProcessRead();
  onFinished();
}

void FileDownloader::onProcessReadError()
{
  if (!m_process) return;

  QByteArray data = m_process->readAllStandardError();
  if (data.size() > 0)
    {
      QString
          err = tr("Error in processing downloaded data") + ": " +
          QString(data.data());
      onError(err);
    }
}

void FileDownloader::onProcessStateChanged(QProcess::ProcessState state)
{
  if ( !m_process_started && state == QProcess::NotRunning )
    {
      QString err = tr("Error in processing downloaded data: could not start the program") + " " + m_process->program();
      onError(err);
    }
}
