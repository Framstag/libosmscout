#ifndef OSMSCOUT_CLIENT_QT_AVAILABLEMAPSMODEL_H
#define OSMSCOUT_CLIENT_QT_AVAILABLEMAPSMODEL_H

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

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>

#include <osmscout/MapProvider.h>
#include <osmscout/Settings.h>

namespace osmscout {

/**
 * Abstract model item used in AvailableMapsModel
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModelItem : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool valid READ isValid() CONSTANT)
  Q_PROPERTY(QString name READ getName() CONSTANT)
  Q_PROPERTY(QStringList path READ getPath() CONSTANT)
  Q_PROPERTY(QString description READ getDescription() CONSTANT)

private:
  bool valid{false};
  QString name;
  QStringList path;
  QString description;

public:
  AvailableMapsModelItem() = default;

  inline AvailableMapsModelItem(QString name, QStringList path, QString description):
    valid(true), name(name), path(path), description(description){};

  inline AvailableMapsModelItem(const AvailableMapsModelItem &o):
    QObject(o.parent()),
    valid(o.valid), name(o.name), path(o.path), description(o.description){};

  ~AvailableMapsModelItem() override = default;

  inline AvailableMapsModelItem& operator=(const AvailableMapsModelItem &o)
  {
    valid=o.valid;
    name=o.name;
    path=o.path;
    description=o.description;
    return *this;
  }

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

  ~AvailableMapsModelDir() override = default;

  inline bool isDirectory() const override
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

  Q_PROPERTY(quint64 byteSize READ getSize() CONSTANT)
  Q_PROPERTY(QString size READ getSizeHuman() CONSTANT)
  Q_PROPERTY(QString serverDirectory READ getServerDirectory() CONSTANT)
  Q_PROPERTY(QDateTime time READ getCreation() CONSTANT)
  Q_PROPERTY(int version READ getVersion() CONSTANT)

private:
  MapProvider provider;
  uint64_t size{0};
  QString serverDirectory;
  QDateTime creation;
  int version{-1};

public:
  AvailableMapsModelMap() = default;

  inline AvailableMapsModelMap(QString name, QList<QString> path, QString description, MapProvider provider,
                               uint64_t size, QString serverDirectory, QDateTime creation, int version):
    AvailableMapsModelItem(name, path, description), provider(provider), size(size), serverDirectory(serverDirectory),
    creation(creation), version(version) {};

  inline AvailableMapsModelMap(const AvailableMapsModelMap &o):
    AvailableMapsModelItem(o), provider(o.provider), size(o.size), serverDirectory(o.serverDirectory),
    creation(o.creation), version(o.version) {};

  ~AvailableMapsModelMap() override = default;

  inline bool isDirectory() const override
  {
    return false;
  };

  inline AvailableMapsModelMap& operator=(const AvailableMapsModelMap &o)
  {
    AvailableMapsModelItem::operator=(o);
    provider=o.provider;
    size=o.size;
    serverDirectory=o.serverDirectory;
    creation=o.creation;
    version=o.version;
    return *this;
  }

  MapProvider getProvider() const;
  uint64_t getSize() const;
  QString getSizeHuman() const;
  QString getServerDirectory() const;
  QDateTime getCreation() const;
  int getVersion() const;
};

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

  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString fetchError READ getFetchError NOTIFY loadingChanged)

signals:
  void loadingChanged();

public slots:
  void listDownloaded(const MapProvider &provider, QNetworkReply*);
  void reload();

public:
  AvailableMapsModel();

  ~AvailableMapsModel() override;

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
  Q_ENUM(Roles)

  Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  Q_INVOKABLE int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  Q_INVOKABLE QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
  Q_INVOKABLE QModelIndex parent(const QModelIndex &index) const override;

  Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;

  Q_INVOKABLE QVariant map(const QModelIndex &index) const;

  /**
   * Generation time of map with given path. Null if don't exists in available maps.
   * It may be used for detection if there is some update available.
   *
   * @param path
   * @return
   */
   Q_INVOKABLE QVariant timeOfMap(QStringList path);
   Q_INVOKABLE QObject* mapByPath(QStringList path);

  inline bool isLoading() const{
    return requests>0;
  }

  inline QString getFetchError(){
    return fetchError;
  }

private:
  void append(AvailableMapsModelItem *item);
  QList<AvailableMapsModelItem *> findChildrenByPath(QStringList dir) const;

  QNetworkAccessManager     webCtrl;
  QNetworkDiskCache         diskCache;
  QList<MapProvider>        mapProviders;
  size_t                    requests{0};
  QList<AvailableMapsModelItem*> items;
  QString                   fetchError;
};

}

Q_DECLARE_METATYPE(osmscout::AvailableMapsModelMap)

#endif	/* OSMSCOUT_CLIENT_QT_AVAILABLEMAPMODEL_H */
