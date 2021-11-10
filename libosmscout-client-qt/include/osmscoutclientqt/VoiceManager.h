#ifndef OSMSCOUT_CLIENT_QT_VOICEMANAGER_H
#define OSMSCOUT_CLIENT_QT_VOICEMANAGER_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2020 Lukas Karas

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

#include <osmscoutclientqt/FileDownloader.h>
#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/Voice.h>

#include <memory>

namespace osmscout {

/**
 * Utility class for downloading voice described by AvailableVoice
 * over http.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API VoiceDownloadJob: public DownloadJob
{
  Q_OBJECT

public:
  VoiceDownloadJob(QNetworkAccessManager *webCtrl,
                   const AvailableVoice &voice,
                   const QDir &target,
                   bool replaceExisting);

  virtual ~VoiceDownloadJob();

  void start();

  inline uint64_t expectedSize() const override
  {
    return 0;
  }

  AvailableVoice getVoice() const
  {
    return voice;
  }

private:
  AvailableVoice voice;
};


/**
 * Manager of voices for navigation commands.
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API VoiceManager: public QObject {
  Q_OBJECT

signals:
  void reloaded();

  void startDownloading(const AvailableVoice &item);
  void downloaded(const AvailableVoice &item);
  void removed(const AvailableVoice &item);

  void voiceDownloadFails(const QString &errorMessage);

public slots:
  void reload();

  void download(const AvailableVoice &item);
  void remove(const AvailableVoice &item);
  void cancelDownload(const AvailableVoice &item);

  void onJobFinished();
  void onJobFailed(QString errorMessage);

public:
  VoiceManager();

  VoiceManager(const VoiceManager&) = delete;
  VoiceManager(VoiceManager&&) = delete;

  VoiceManager& operator=(const VoiceManager&) = delete;
  VoiceManager& operator=(VoiceManager&&) = delete;

  ~VoiceManager() override;

  QList<Voice> getInstalledVoices() const
  {
    return installedVoices;
  }

  bool isDownloaded(const AvailableVoice &voice) const;
  bool isDownloading(const AvailableVoice &voice) const;

  void downloadNext();

private:
  QList<Voice> installedVoices;
  QList<VoiceDownloadJob*> downloadJobs;
  QNetworkAccessManager webCtrl;
  QString lookupDir;
};

/**
 * \ingroup QtAPI
 */
using VoiceManagerRef = std::shared_ptr<VoiceManager>;

}
#endif //OSMSCOUT_CLIENT_QT_VOICEMANAGER_H
