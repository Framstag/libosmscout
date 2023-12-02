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

#include <osmscout/util/String.h>

#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/AvailableMapsModel.h>
#include <osmscoutclientqt/PersistentCookieJar.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <QString>
#include <QtAlgorithms>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

namespace osmscout {

MapProvider AvailableMapsModelMap::getProvider() const
{
  return provider;
}

uint64_t AvailableMapsModelMap::getSize() const
{
  return size;
}

QString AvailableMapsModelMap::getSizeHuman() const
{
  return QString::fromStdString(osmscout::ByteSizeToString((double)size));
}

QString AvailableMapsModelMap::getServerDirectory() const
{
  return serverDirectory;
}

QDateTime AvailableMapsModelMap::getCreation() const
{
  return creation;
}

int AvailableMapsModelMap::getVersion() const
{
  return version;
}

AvailableMapsModel::AvailableMapsModel()
{
  SettingsRef settings = OSMScoutQt::GetInstance().GetSettings();
  auto providers = settings->GetMapProviders();
  for (const auto &provider: providers) {
    mapProviders << provider;
  }

  diskCache.setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "OSMScoutHttpCache");
  webCtrl.setCache(&diskCache);
  webCtrl.setCookieJar(new PersistentCookieJar(settings));

  reload();
}

void AvailableMapsModel::reload()
{
  fetchError=""; // reset errors

  QLocale locale;
  for (const auto &provider: mapProviders){
    std::string url = provider.getListUri(osmscout::TypeConfig::MIN_FORMAT_VERSION,
                                          osmscout::TypeConfig::MAX_FORMAT_VERSION,
                                          locale.name().toStdString());
    QNetworkRequest request(QUrl(QString::fromStdString(url)));

    request.setHeader(QNetworkRequest::UserAgentHeader, OSMScoutQt::GetInstance().GetUserAgent());

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0) /* For compatibility with QT 5.6 */
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
    //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    QNetworkReply *reply = webCtrl.get(request);
    requests++;
    connect(reply, &QNetworkReply::finished, [provider, this, reply](){
      this->listDownloaded(provider, reply);
    });
  }
  emit loadingChanged();
}

AvailableMapsModel::~AvailableMapsModel()
{
  for (auto *item:items){
    delete item;
  }
  items.clear();
}
void AvailableMapsModel::append(AvailableMapsModelItem *item)
{
  if (!item->isDirectory()){
    items.append(item);
  }else{
    // avoid duplicate directories
    for (auto *e:items){
      if (e->isDirectory() && e->getPath() == item->getPath()){
        return;
      }
    }
    items.append(item);
  }
}

bool availableMapsModelItemLessThan(const AvailableMapsModelItem *i1, const AvailableMapsModelItem *i2)
{
  if (i1->isDirectory() && !i2->isDirectory()) {
    return true;
  }

  if (i2->isDirectory() && !i1->isDirectory()) {
    return false;
  }

  return i1->getName().localeAwareCompare(i2->getName()) < 0;
}

void AvailableMapsModel::listDownloaded(const MapProvider &provider, QNetworkReply* reply)
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
    for (const QJsonValueRef ref: doc.array()){
      if (!ref.isObject())
        continue;
      QJsonObject obj=ref.toObject();
      auto dir=obj.value("dir");
      auto name=obj.value("name");
      auto description=obj.value("description");
      if (!name.isString())
        continue;
      if (dir.isString()){
        append(new AvailableMapsModelDir(name.toString(), dir.toString().split('/'), description.toString()));
      }else{
        auto map=obj.value("map");
        auto sizeObj=obj.value("size");
        auto serverDirectory=obj.value("directory");
        auto timestamp=obj.value("timestamp");
        auto version=obj.value("version");

        double size=0;
        if (sizeObj.isDouble()){
          size=sizeObj.toDouble();
        }else if (sizeObj.isString()){
          size=sizeObj.toString().toDouble();
        }
        if (map.isString() && serverDirectory.isString() &&
            timestamp.isDouble() && version.isDouble()){

          QDateTime creation;
#if QT_VERSION > QT_VERSION_CHECK(5, 8, 0) /* For compatibility with QT 5.6 */
          creation.setSecsSinceEpoch(timestamp.toDouble());
#else
          creation.setTime_t(timestamp.toDouble());
#endif

          append(new AvailableMapsModelMap(name.toString(), map.toString().split('/'), description.toString(),
                                           provider, size, serverDirectory.toString(),
                                           creation, version.toDouble()));
        }else{
          qWarning() << "Invalid item:" << obj;
        }
      }
    }
  }

  std::sort(items.begin(), items.end(), availableMapsModelItemLessThan);
  reply->deleteLater();

  emit loadingChanged();
  endResetModel();
}

QList<AvailableMapsModelItem*> AvailableMapsModel::findChildrenByPath(QStringList dir) const
{
  QList<AvailableMapsModelItem*> result;
  for (auto *item:items){
    auto path=item->getPath();
    if (path.size() == dir.size()+1){
      bool match=true;
      for (int i=0; i<dir.size(); i++){
        if (path[i]!=dir[i]){
          match=false;
          break;
        }
      }
      if (match){
        result.append(item);
      }
    }
  }
  return result;
}

QModelIndex AvailableMapsModel::index(int row, int column, const QModelIndex &parent) const
{
  QStringList parentPath;
  if (parent.isValid()){
    AvailableMapsModelItem* item = static_cast<AvailableMapsModelItem*>(parent.internalPointer());
    parentPath=item->getPath();
  }
  QList<AvailableMapsModelItem*> children=findChildrenByPath(parentPath);
  if (row<children.size()){
    return createIndex(row, column, children[row]);
  }
  qWarning() << "Can't find item on row" << row << "parent" << parent;
  return QModelIndex(); // should not happen
}

QModelIndex AvailableMapsModel::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();

  AvailableMapsModelItem *childItem = static_cast<AvailableMapsModelItem*>(index.internalPointer());
  QStringList parentDir=childItem->getPath();

  if (parentDir.isEmpty())
    return QModelIndex();
  parentDir.removeLast();
  QStringList parentPath=parentDir;
  if (parentPath.isEmpty())
    return QModelIndex();
  parentDir.removeLast();

  QList<AvailableMapsModelItem*> parentSiblings=findChildrenByPath(parentDir);
  for (int row=0;row<parentSiblings.size();row++){
    auto *parentCandidate=parentSiblings[row];
    if (parentCandidate->getPath()==parentPath)
      return createIndex(row, 0, parentCandidate);
  }
  return QModelIndex(); // should not happen

}

int AvailableMapsModel::rowCount(const QModelIndex &parentIndex) const
{
  QStringList dir;
  if (parentIndex.isValid()){
    AvailableMapsModelItem *parent=static_cast<AvailableMapsModelItem*>(parentIndex.internalPointer());
    dir=parent->getPath();
  }
  QList<AvailableMapsModelItem*> candidates=findChildrenByPath(dir);
  return candidates.size();
}

int AvailableMapsModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid()){
    return 1;
  }
  return 0;
}

QVariant AvailableMapsModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid()){
    return QVariant();
  }
  const AvailableMapsModelItem *item=static_cast<AvailableMapsModelItem*>(index.internalPointer());
  const AvailableMapsModelMap *map=dynamic_cast<const AvailableMapsModelMap*>(item);

  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return item->getName();
    case PathRole:
      return item->getPath(); // path in tree
    case DirRole:
      return item->isDirectory(); // isDir? true: false
    case ServerDirectoryRole:
      return map==nullptr ? QVariant(): map->getServerDirectory();// server path for this map
    case TimeRole:
      return map==nullptr ? QVariant(): map->getCreation();// QTime of map creation
    case VersionRole:
      return map==nullptr ? QVariant(): map->getVersion();
    case ByteSizeRole:
      return map==nullptr ? QVariant(): QVariant((double)map->getSize());
    case SizeRole:
      return map==nullptr ? "": QVariant(map->getSizeHuman());
    case ProviderUriRole:
      return map==nullptr ? QVariant(): QString::fromStdString(map->getProvider().getName());
    case DescriptionRole:
      return item->getDescription();
    case MapRole:
      return QVariant::fromValue(map==nullptr ? nullptr: new AvailableMapsModelMap(*map));
    default:
        break;
  }
  return QVariant();
}

QVariant AvailableMapsModel::map(const QModelIndex &index) const
{
  return data(index,MapRole);
}

QHash<int, QByteArray> AvailableMapsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractItemModel::roleNames();

  roles[NameRole]="name";
  roles[PathRole]="path";
  roles[DirRole]="dir";
  roles[ServerDirectoryRole]="serverDirectory";
  roles[TimeRole]="time";
  roles[VersionRole]="version";
  roles[ByteSizeRole]="byteSize";
  roles[SizeRole]="size";
  roles[ProviderUriRole]="providerUri";
  roles[DescriptionRole]="description";
  roles[MapRole]="map";

  return roles;
}

Qt::ItemFlags AvailableMapsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant AvailableMapsModel::timeOfMap(QStringList path)
{
  if (path.empty()){
    return QVariant();
  }
  for (const AvailableMapsModelItem* item:items){
    if (item->isDirectory() || !item->isValid()){
      continue;
    }
    if (item->getPath()==path){
      const AvailableMapsModelMap *map=dynamic_cast<const AvailableMapsModelMap*>(item);
      if (map != nullptr){
        return map->getCreation();
      }
    }
  }

  return QVariant();
}

QObject* AvailableMapsModel::mapByPath(QStringList path)
{
  if (path.empty()){
    return nullptr;
  }
  for (const AvailableMapsModelItem* item:items){
    if (item->isDirectory() || !item->isValid()){
      continue;
    }
    if (item->getPath()==path){
      const AvailableMapsModelMap *map=dynamic_cast<const AvailableMapsModelMap*>(item);
      if (map != nullptr){
        return new AvailableMapsModelMap(*map);
      }
    }
  }

  return nullptr;
}
}
