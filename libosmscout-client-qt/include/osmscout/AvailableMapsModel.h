/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2016 Lukas Karas

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

#ifndef AVAILABLEMAPSMODEL_H
#define	AVAILABLEMAPSMODEL_H

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>

#include <osmscout/MapProvider.h>
#include <osmscout/Settings.h>

/**
 * Abstract model item used in AvailableMapsModel
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModelItem : public QObject {
  Q_OBJECT

private:
  bool valid;
  QString name;
  QStringList path;
  QString description;

public:
  inline AvailableMapsModelItem():valid(false){};
  inline AvailableMapsModelItem(QString name, QStringList path, QString description):
    valid(true), name(name), path(path), description(description){};

  inline AvailableMapsModelItem(const AvailableMapsModelItem &o):
    QObject(o.parent()),
    name(o.name), path(o.path), description(o.description){};  

  virtual inline ~AvailableMapsModelItem(){}
    
  inline QString getName() const
  {
    return name;
  }
    
  inline QStringList getPath() const
  {
    return path;
  }
  
  inline QString getDescription() const
  {
    return description;
  }
  
  inline bool isValid() const 
  {
    return valid;
  }
  
  virtual bool isDirectory() const = 0;
};

/**
 * Model item representing directory in AvailableMapsModel
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModelDir : public AvailableMapsModelItem {
  Q_OBJECT

public: 
  inline AvailableMapsModelDir(QString name, QList<QString> path, QString description):
    AvailableMapsModelItem(name, path, description){};

  inline AvailableMapsModelDir(const AvailableMapsModelDir &o):
    AvailableMapsModelItem(o){};  
    
  virtual inline ~AvailableMapsModelDir(){};

  virtual inline bool isDirectory() const
  {
    return true;
  };
};

/**
 * Model item representing map in AvailableMapsModel
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModelMap : public AvailableMapsModelItem {
  Q_OBJECT
  
private:
  MapProvider provider;
  size_t size;
  QString serverDirectory;
  QDateTime creation;
  int version;

public: 
  inline AvailableMapsModelMap():AvailableMapsModelItem(){};

  inline AvailableMapsModelMap(QString name, QList<QString> path, QString description, MapProvider provider,
                               size_t size, QString serverDirectory, QDateTime creation, int version):
    AvailableMapsModelItem(name, path, description), provider(provider), size(size), serverDirectory(serverDirectory), 
    creation(creation), version(version) {};

  inline AvailableMapsModelMap(const AvailableMapsModelMap &o):
    AvailableMapsModelItem(o), provider(o.provider), size(o.size), serverDirectory(o.serverDirectory), 
    creation(o.creation), version(o.version) {};  
    
  virtual inline ~AvailableMapsModelMap(){};

  virtual inline bool isDirectory() const
  {
    return false;
  };
  
  MapProvider getProvider() const;
  size_t getSize() const;
  QString getSizeHuman() const;
  QString getServerDirectory() const;
  QDateTime getCreation() const;
  int getVersion() const;
};

Q_DECLARE_METATYPE(AvailableMapsModelMap)

/**
 * Tree model with maps available by configured providers (see Settings::GetMapProviders).
 * Every map provider have to expose list of maps by json. Json format exammple:
 * 
 * <pre>
 * [
 *  {
 *    "version" : 10,
 *    "timestamp" : 1480801927,
 *    "name" : "Czech Republic",
 *    "directory" : "europe/czech-republic-10-20161203",
 *    "size" : 622036876,
 *    "map" : "europe/czech-republic"
 *  },
 *  {
 *    "dir" : "europe",
 *    "name" : "Europe"
 *  }
 * ]
 * </pre>
 * 
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModel : public QAbstractItemModel {
  Q_OBJECT
  
  Q_PROPERTY(bool loading READ isLoading NOTIFY loaded)

signals:
  void loaded();

public slots:
  void listDownloaded(QNetworkReply*);

public:
  AvailableMapsModel();

  virtual ~AvailableMapsModel();

  enum Roles {
    NameRole = Qt::UserRole, // localized name
    PathRole = Qt::UserRole+1, // path in tree
    DirRole = Qt::UserRole+2, // isDir? true: false
    ServerDirectoryRole = Qt::UserRole+3, // server path for this map
    TimeRole = Qt::UserRole+4, // QTime of map creation 
    VersionRole = Qt::UserRole+5,
    ByteSizeRole = Qt::UserRole+6,
    ProviderUriRole = Qt::UserRole+7,
    DescriptionRole = Qt::UserRole+8,
    SizeRole = Qt::UserRole+9,
    MapRole = Qt::UserRole+10,
  };

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QModelIndex parent(const QModelIndex &index) const;

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  
  Q_INVOKABLE QVariant map(const QModelIndex &index) const;

  inline bool isLoading(){
    return !requests.isEmpty();
  }

private:
  void append(AvailableMapsModelItem *item);
  QList<AvailableMapsModelItem *> findChildrenByPath(QStringList dir) const;

  QNetworkAccessManager     webCtrl; 
  QNetworkDiskCache         diskCache;
  QList<MapProvider>        mapProviders;
  QHash<QUrl,MapProvider>   requests;
  QList<AvailableMapsModelItem*> items;
};

#endif	/* AVAILABLEMAPMODEL_H */
