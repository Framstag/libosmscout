#ifndef OSMSCOUT_CLIENT_QT_VAILABLEVOICESMODEL_H
#define OSMSCOUT_CLIENT_QT_VAILABLEVOICESMODEL_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclient/VoiceProvider.h>

#include <osmscoutclientqt/VoiceManager.h>
#include <osmscoutclientqt/Voice.h>

#include <QAbstractListModel>
#include <QNetworkDiskCache>
#include <QNetworkAccessManager>

namespace osmscout {

/**
 * List model with voices available by configured providers (see Settings::GetVoiceProviders).
 * Every voice provider have to expose list of voices by json. Json format exammple:
 *
 * <pre>
 * [
 *  {
 *    "lang": "American English",
 *    "gender": "female",
 *    "name": "Alex",
 *    "license": "CC-By-SA 3.0",
 *    "dir": "American English - Alex (female)",
 *    "author": "Alex Spehr",
 *    "description": "American English speaker"
 *  } 
 * ]
 * </pre>
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableVoicesModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString fetchError READ getFetchError NOTIFY loadingChanged)

signals:
  void loadingChanged();

public slots:
  void listDownloaded(const VoiceProvider &provider, QNetworkReply*);
  void reload();

  void onVoiceStateChanged(const AvailableVoice &voice);

public:
  AvailableVoicesModel();

  virtual ~AvailableVoicesModel();

  enum Roles {
    NameRole = Qt::UserRole,
    LangRole = Qt::UserRole + 1,
    GenderRole = Qt::UserRole + 2,
    LicenseRole = Qt::UserRole + 3,
    DirectoryRole = Qt::UserRole + 4,
    AuthorRole = Qt::UserRole + 5,
    DescriptionRole = Qt::UserRole + 6,
    StateRole = Qt::UserRole + 7
  };
  Q_ENUM(Roles)

  enum VoiceState {
    Available,
    Downloading,
    Downloaded
  };
  Q_ENUM(VoiceState)

  Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;

  Q_INVOKABLE void download(const QModelIndex &index);
  Q_INVOKABLE void remove(const QModelIndex &index);

  Q_INVOKABLE QString stateStr(VoiceState state) const;

  inline bool isLoading(){
    return requests>0;
  }

  inline QString getFetchError(){
    return fetchError;
  }

private:
  int findRow(const QString &lang, const QString &name);

private:
  VoiceManagerRef           voiceManager;
  QNetworkAccessManager     webCtrl;
  QNetworkDiskCache         diskCache;
  QList<VoiceProvider>      voiceProviders;
  size_t                    requests{0};
  QList<AvailableVoice*>    items;
  QString                   fetchError;
};

}

#endif //OSMSCOUT_CLIENT_QT_VAILABLEVOICESMODEL_H
