/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Lukáš Karas

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

#include <osmscout/AvailableMapsModel.h>

AvailableMapsModel::AvailableMapsModel()
{
  mapProviders = Settings::GetInstance()->GetMapProviders();

  connect(&webCtrl, SIGNAL (finished(QNetworkReply*)),  this, SLOT(listDownloaded(QNetworkReply*)));    
  diskCache.setCacheDirectory(Settings::GetInstance()->GetHttpCacheDir());
  webCtrl.setCache(&diskCache);  

  QLocale locale;
  for (auto &provider: mapProviders){
    QUrl url = provider.getListUri(osmscout::TypeConfig::MIN_FORMAT_VERSION,
                                   osmscout::TypeConfig::MAX_FORMAT_VERSION, 
                                   locale.name());
    QNetworkRequest request(url);
    requests[url]=provider;
    
    request.setHeader(QNetworkRequest::UserAgentHeader, QString(OSMSCOUT_USER_AGENT).arg(OSMSCOUT_VERSION_STRING));
    //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    webCtrl.get(request);
  }
}

void AvailableMapsModel::listDownloaded(QNetworkReply* reply)
{
  QUrl url = reply->url();
  if (!requests.contains(url)){
    qWarning() << "Response from non-requested url: " << url;
  }else{
    MapProvider provider=requests.value(url);
    requests.remove(url);
    if (reply->error() != QNetworkReply::NoError){
      qWarning() << "Downloading " << url << "failed with " << reply->errorString();
    }else{
      QByteArray downloadedData = reply->readAll();
      QJsonDocument doc = QJsonDocument::fromJson(downloadedData);
      for (auto &obj: doc.array()){
          // TODO: load JSON
          //MapProvider provider = MapProvider::fromJson(obj);
      }
    }
  }
  reply->deleteLater();
}

int AvailableMapsModel::rowCount(const QModelIndex &parent) const
{
  return 0; // TODO
}

QVariant AvailableMapsModel::data(const QModelIndex &index, int role) const
{
  return QVariant(); // TODO
}

QHash<int, QByteArray> AvailableMapsModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[NameRole]="name";
    roles[PathRole]="path";
    roles[DirRole]="dir";
    roles[ServerDirectoryRole]="serverDirectory";
    roles[TimeRole]="time";
    roles[VersionRole]="version";
    roles[SizeRole]="size";
    roles[ProviderUriRole]="providerUri";

    return roles;
}

Qt::ItemFlags AvailableMapsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
