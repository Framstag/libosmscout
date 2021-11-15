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

#include <osmscoutclientqt/VoiceManager.h>
#include <osmscoutclientqt/Settings.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/PersistentCookieJar.h>
#include <osmscoutclientqt/AvailableVoicesModel.h>

namespace osmscout {

VoiceDownloadJob::VoiceDownloadJob(QNetworkAccessManager *webCtrl,
                                   const AvailableVoice &voice,
                                   const QDir &target,
                                   bool replaceExisting):
    DownloadJob(webCtrl, target, replaceExisting), voice(voice)
{}

VoiceDownloadJob::~VoiceDownloadJob()
{
  if (started && !successful){
    // delete partial voice
    Voice dir(target);
    dir.deleteVoice();
  }
}

void VoiceDownloadJob::start()
{
  if (target.exists() && !isReplaceExisting()) {
    qWarning() << "Directory already exists"<<target.canonicalPath()<<"!";
    onJobFailed("Directory already exists", false);
    return;
  }

  if (!target.mkpath(target.path())) {
    qWarning() << "Can't create directory" << target.canonicalPath() << "!";
    onJobFailed("Can't create directory", false);
    return;
  }

  started=true;
  QJsonObject metadata;
  metadata["lang"] = voice.getLang();
  metadata["gender"] = voice.getGender();
  metadata["name"] = voice.getName();
  metadata["license"] = voice.getLicense();
  metadata["author"] = voice.getAuthor();
  metadata["description"] = voice.getDescription();

  QJsonDocument doc(metadata);
  QFile metadataFile(target.filePath(MapDownloadJob::FILE_METADATA));
  metadataFile.open(QFile::OpenModeFlag::WriteOnly);
  metadataFile.write(doc.toJson());
  metadataFile.close();
  if (metadataFile.error() != QFile::FileError::NoError){
    done = true;
    error = metadataFile.errorString();
    emit failed(metadataFile.errorString());
    return;
  }

  DownloadJob::start(voice.getProvider().getUri()+"/"+voice.getDirectory(), Voice::files());
}

VoiceManager::VoiceManager()
{
  SettingsRef settings = OSMScoutQt::GetInstance().GetSettings();

  webCtrl.setCookieJar(new PersistentCookieJar(settings));
  // we don't use disk cache here

  reload();
}

void VoiceManager::reload()
{
  SettingsRef settings = OSMScoutQt::GetInstance().GetSettings();
  lookupDir = settings->GetVoiceLookupDirectory();
  osmscout::log.Info() << "Lookup voices at " << lookupDir.toStdString();
  installedVoices.clear();
  QSet<QString> uniqPaths;
  if (QDir(lookupDir).exists()) {
    QDirIterator dirIt(lookupDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (dirIt.hasNext()) {
      dirIt.next();
      QFileInfo fInfo(dirIt.filePath());
      QString path = fInfo.absoluteFilePath();
      if (fInfo.isDir() && !uniqPaths.contains(path)){
        uniqPaths << path;
        Voice voiceDir(path);
        if (voiceDir.isValid()) {
          osmscout::log.Info() << "found voice " << voiceDir.getName().toStdString() << ": " << path.toStdString();
          installedVoices << voiceDir;
        }
      }
    }
  }
  emit reloaded();

  // check if configured voice still exists
  QString voiceDir = settings->GetVoiceDir();
  if (!voiceDir.isEmpty()){
    bool found=false;
    for (const auto &v:installedVoices){
      if (voiceDir==v.getDir().absolutePath()){
        found = true;
        break;
      }
    }
    if (!found){
      log.Warn() << "Voice " << voiceDir.toStdString() << " don't exists anymore, reset";
      settings->SetVoiceDir("");
    }
  }
}

VoiceManager::~VoiceManager()
{
  for (auto* job:downloadJobs) {
    delete job;
  }
  downloadJobs.clear();
}

bool VoiceManager::isDownloaded(const AvailableVoice &voice) const
{
  for (const auto &v: installedVoices){
    if (v.getName() == voice.getName() && v.getLang() == voice.getLang()) {
      return true;
    }
  }
  return false;
}

bool VoiceManager::isDownloading(const AvailableVoice &voice) const
{
  for (const auto *j: downloadJobs){
    const auto v=j->getVoice();
    if (v.getName() == voice.getName() && v.getLang() == voice.getLang()) {
      return true;
    }
  }
  return false;
}

void VoiceManager::download(const AvailableVoice &voice)
{
  auto* job=new VoiceDownloadJob(&webCtrl,
      voice,
      lookupDir + QDir::separator() + voice.getDirectory(),
      /*replaceExisting*/ true);

  connect(job, &VoiceDownloadJob::finished, this, &VoiceManager::onJobFinished);
  connect(job, &VoiceDownloadJob::canceled, this, &VoiceManager::onJobFinished);
  connect(job, &VoiceDownloadJob::failed, this, &VoiceManager::onJobFailed);
  downloadJobs<<job;
  emit startDownloading(voice);
  downloadNext();
}

void VoiceManager::downloadNext()
{
  for (const auto* job:downloadJobs){
    if (job->isDownloading()){
      return;
    }
  }
  for (auto* job:downloadJobs){
    job->start();
    break;
  }
}

void VoiceManager::onJobFailed(QString errorMessage)
{
  onJobFinished();
  emit voiceDownloadFails(errorMessage);
}

void VoiceManager::onJobFinished()
{
  QList<VoiceDownloadJob*> finished;
  for (auto *job:downloadJobs){
    if (job->isDone()){
      finished << job;
    }
  }
  if (!finished.isEmpty()){
    reload();
  }
  for (auto *job:finished){
    downloadJobs.removeOne(job);
    emit downloaded(job->getVoice());
    job->deleteLater();
  }
  downloadNext();
}

void VoiceManager::remove(const AvailableVoice &voice)
{
  bool changed=false;
  for (auto &v: installedVoices){
    if (v.getName() == voice.getName() && v.getLang() == voice.getLang()) {
      if (!v.deleteVoice()){
        qWarning() << "Failed to remove " << v.getDir().absolutePath();
      }
      changed=true;
    }
  }
  if (changed){
    reload();
    emit removed(voice);
  }
}

void VoiceManager::cancelDownload(const AvailableVoice &voice)
{
  for (auto *job: downloadJobs){
    const auto v=job->getVoice();
    if (v.getName() == voice.getName() && v.getLang() == voice.getLang()) {
      qDebug() << "Cancel downloading voice:" << v.getName();
      job->cancel();
    }
  }
}

}
