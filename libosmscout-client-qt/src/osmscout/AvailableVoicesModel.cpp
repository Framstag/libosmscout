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

#include <osmscout/AvailableVoicesModel.h>
#include <osmscout/PersistentCookieJar.h>
#include <osmscout/OSMScoutQt.h>

#include <algorithm>

namespace osmscout {

AvailableVoicesModel::AvailableVoicesModel()
{
  SettingsRef settings = OSMScoutQt::GetInstance().GetSettings();
  assert(settings);
  voiceProviders = settings->GetVoiceProviders();

  diskCache.setCacheDirectory(settings->GetHttpCacheDir());
  webCtrl.setCache(&diskCache);
  webCtrl.setCookieJar(new PersistentCookieJar(settings));

  voiceManager = OSMScoutQt::GetInstance().GetVoiceManager();
  assert(voiceManager);

  connect(voiceManager.get(), &VoiceManager::startDownloading,
          this, &AvailableVoicesModel::onVoiceStateChanged);
  connect(voiceManager.get(), &VoiceManager::downloaded,
          this, &AvailableVoicesModel::onVoiceStateChanged);
  connect(voiceManager.get(), &VoiceManager::removed,
          this, &AvailableVoicesModel::onVoiceStateChanged);

  reload();
}

AvailableVoicesModel::~AvailableVoicesModel()
{
  for (auto item:items){
    delete item;
  }
  items.clear();
}

void AvailableVoicesModel::reload()
{
  fetchError=""; // reset errors

  beginResetModel();
  for (auto item:items){
    delete item;
  }
  items.clear();
  endResetModel();

  QLocale locale;
  for (auto &provider: voiceProviders){
    QUrl url = provider.getListUri(locale.name());
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::UserAgentHeader, OSMScoutQt::GetInstance().GetUserAgent());
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    QNetworkReply *reply = webCtrl.get(request);
    requests++;
    connect(reply, &QNetworkReply::finished, [provider, this, reply](){
      this->listDownloaded(provider, reply);
    });
  }
  emit loadingChanged();
}

namespace {
bool availableVoiceItemLessThan(const AvailableVoice *i1, const AvailableVoice *i2)
{
  assert(i1);
  assert(i2);
  if (i1->getLang() != i2->getLang()) {
    return i1->getLang().localeAwareCompare(i2->getLang()) < 0;
  }
  return i1->getName().localeAwareCompare(i2->getName()) < 0;
}
}

void AvailableVoicesModel::listDownloaded(const VoiceProvider &provider, QNetworkReply* reply)
{
  beginResetModel();
  requests--;

  QUrl url = reply->url();
  if (reply->error() != QNetworkReply::NoError){
    qWarning() << "Downloading " << url << "failed with " << reply->errorString();
    fetchError=reply->errorString();
  }else{
    QByteArray downloadedData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(downloadedData);
    for (const QJsonValueRef &ref: doc.array()){
      if (!ref.isObject())
        continue;
      QJsonObject obj=ref.toObject();

      auto lang=obj.value("lang");
      auto gender=obj.value("gender");
      auto name=obj.value("name");
      auto license=obj.value("license");
      auto dir=obj.value("dir");
      auto author=obj.value("author");
      auto description=obj.value("description");

      if (!lang.isString() ||
          !gender.isString() ||
          !name.isString() ||
          !license.isString() ||
          !dir.isString() ||
          !author.isString() ||
          !description.isString()) {

        qWarning() << "Invalid item:" << obj;
        continue;
      }

      items.append(new AvailableVoice(
          provider,
          lang.toString(),
          gender.toString(),
          name.toString(),
          license.toString(),
          dir.toString(),
          author.toString(),
          description.toString()));
    }
  }

  std::sort(items.begin(), items.end(), availableVoiceItemLessThan);
  reply->deleteLater();

  // TODO: add locally installed voices to be able uninstall them

  emit loadingChanged();
  endResetModel();
}

int AvailableVoicesModel::rowCount(const QModelIndex &) const
{
  return items.size();
}

QString AvailableVoicesModel::stateStr(VoiceState state) const
{
  switch (state){
    case Available: return "Available";
    case Downloading: return "Downloading";
    case Downloaded: return "Downloaded";
    default:
      assert(false);
      return "";
  }
}

QVariant AvailableVoicesModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()){
    return QVariant();
  }
  if(index.row() < 0 || index.row() >= items.size()) {
    return QVariant();
  }

  const AvailableVoice *item=items.at(index.row());

  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return item->getName();
    case LangRole:
      return item->getLang();
    case GenderRole:
      return item->getGender();
    case LicenseRole:
      return item->getLicense();
    case DirectoryRole:
      return item->getDirectory();
    case AuthorRole:
      return item->getAuthor();
    case DescriptionRole:
      return item->getDescription();
    case StateRole:
      return voiceManager->isDownloaded(*item) ? Downloaded :
             voiceManager->isDownloading(*item) ? Downloading : Available;
    default:
      break;
  }
  return QVariant();
}

int AvailableVoicesModel::findRow(const QString &lang, const QString &name)
{
  int row=0;
  for (const auto *v:items){
    if (name == v->getName() && lang == v->getLang()){
      return row;
    }
    row += 1;
  }
  return -1;
}

void AvailableVoicesModel::onVoiceStateChanged(const AvailableVoice &voice)
{
  int row=findRow(voice.getLang(), voice.getName());
  if (row<0) {
    return;
  }

  QVector<int> roles;
  roles<<StateRole;
  emit dataChanged(createIndex(row,0), createIndex(row,0),roles);
}

void AvailableVoicesModel::download(const QModelIndex &index)
{
  if (!index.isValid() || index.row() < 0 || index.row() >= items.size()) {
    return;
  }
  const AvailableVoice *item=items.at(index.row());
  if (!voiceManager->isDownloaded(*item) &&
      !voiceManager->isDownloading(*item)) {
    voiceManager->download(*item);
  }
}

void AvailableVoicesModel::remove(const QModelIndex &index)
{
  if (!index.isValid() || index.row() < 0 || index.row() >= items.size()) {
    return;
  }
  const AvailableVoice *item=items.at(index.row());
  if (voiceManager->isDownloaded(*item)) {
    voiceManager->remove(*item);
  } else if (voiceManager->isDownloading(*item)) {
    voiceManager->cancelDownload(*item);
  }
}

QHash<int, QByteArray> AvailableVoicesModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole]="name";
  roles[LangRole]="lang";
  roles[GenderRole]="gender";
  roles[LicenseRole]="license";
  roles[DirectoryRole]="directory";
  roles[AuthorRole]="author";
  roles[DescriptionRole]="description";
  roles[StateRole]="state";

  return roles;
}

Qt::ItemFlags AvailableVoicesModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}
