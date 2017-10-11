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

#include <QString>
#include <QtAlgorithms>

#include <osmscout/util/String.h>
#include <osmscout/AvailableMapsModel.h>
#include <osmscout/PersistentCookieJar.h>
#include <osmscout/DBThread.h>
#include <osmscout/OSMScoutQt.h>

MapProvider AvailableMapsModelMap::getProvider() const
{
  return provider;
}

size_t AvailableMapsModelMap::getSize() const
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
  mapProviders = settings->GetMapProviders();

  connect(&webCtrl, SIGNAL (finished(QNetworkReply*)),  this, SLOT(listDownloaded(QNetworkReply*)));
  diskCache.setCacheDirectory(settings->GetHttpCacheDir());
  webCtrl.setCache(&diskCache);
  webCtrl.setCookieJar(new PersistentCookieJar(settings));

  QLocale locale;
  for (auto &provider: mapProviders){
    QUrl url = provider.getListUri(osmscout::TypeConfig::MIN_FORMAT_VERSION,
                                   osmscout::TypeConfig::MAX_FORMAT_VERSION, 
                                   locale.name());
    QNetworkRequest request(url);
    requests[url]=provider;
    
    request.setHeader(QNetworkRequest::UserAgentHeader, OSMScoutQt::GetInstance().GetUserAgent());
    //request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    webCtrl.get(request);
  }
}

AvailableMapsModel::~AvailableMapsModel()
{
  for (auto item:items){
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
    for (auto e:items){
      if (e->isDirectory() && e->getPath() == item->getPath()){
        return;
      }
    }
    items.append(item);
  }
}

bool itemLessThan(const AvailableMapsModelItem *i1, const AvailableMapsModelItem *i2)
{
  if (i1->isDirectory() && !i2->isDirectory())
    return true;
  if (i2->isDirectory() && !i1->isDirectory())
    return false;

  return i1->getName().localeAwareCompare(i2->getName()) < 0;
}

void AvailableMapsModel::listDownloaded(QNetworkReply* reply)
{
  beginResetModel();
  
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
      for (const QJsonValueRef &ref: doc.array()){
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
            creation.setTime_t(timestamp.toDouble());
            
            append(new AvailableMapsModelMap(name.toString(), map.toString().split('/'), description.toString(), 
                                             provider, size, serverDirectory.toString(), 
                                             creation, version.toDouble()));
          }else{
            qWarning() << "Invalid item:" << obj;
          }
        }
      }
    }
  }
  qSort(items.begin(), items.end(), itemLessThan);
  reply->deleteLater();
  
  emit loaded();
  endResetModel();
}

QList<AvailableMapsModelItem*> AvailableMapsModel::findChildrenByPath(QStringList dir) const
{
  QList<AvailableMapsModelItem*> result;
  for (auto item:items){
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
    auto parentCandidate=parentSiblings[row];
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
      return map==NULL ? QVariant(): map->getServerDirectory();// server path for this map
    case TimeRole:
      return map==NULL ? QVariant(): map->getCreation();// QTime of map creation 
    case VersionRole:
      return map==NULL ? QVariant(): map->getVersion();
    case ByteSizeRole:
      return map==NULL ? QVariant(): QVariant((double)map->getSize());
    case SizeRole:
      return map==NULL ? "": QVariant(map->getSizeHuman());
    case ProviderUriRole:
      return map==NULL ? QVariant(): map->getProvider().getName();
    case DescriptionRole:
      return item->getDescription();
    case MapRole:
      return map==NULL ? QVariant(): qVariantFromValue(AvailableMapsModelMap(*map));
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
