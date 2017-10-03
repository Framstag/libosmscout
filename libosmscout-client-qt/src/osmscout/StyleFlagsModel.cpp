/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/StyleFlagsModel.h>
#include <osmscout/DBThread.h>
#include <osmscout/OSMScoutQt.h>

StyleFlagsModel::StyleFlagsModel():
  QAbstractListModel()
{
  styleModule=OSMScoutQt::GetInstance().MakeStyleModule();

  connect(styleModule,SIGNAL(styleFlagsChanged(QMap<QString,bool>)),
          this,SLOT(onStyleFlagsChanged(QMap<QString,bool>)),
          Qt::QueuedConnection);
  connect(this,SIGNAL(styleFlagsRequested()),
          styleModule,SLOT(onStyleFlagsRequested()),
          Qt::QueuedConnection);
  connect(this,SIGNAL(setFlagRequest(QString,bool)),
          styleModule,SLOT(onSetFlagRequest(QString,bool)),
          Qt::QueuedConnection);
  connect(styleModule,SIGNAL(flagSet(QString,bool)),
          this,SLOT(onFlagSet(QString,bool)),
          Qt::QueuedConnection);

  emit styleFlagsRequested();
}

StyleFlagsModel::~StyleFlagsModel()
{
  if (styleModule!=NULL){
    styleModule->deleteLater();
    styleModule=NULL;
  }
}

void StyleFlagsModel::onStyleFlagsChanged(QMap<QString,bool> flags)
{
  beginResetModel();
  mapFlags=flags;
  endResetModel();
}

QVariant StyleFlagsModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= mapFlags.size()) {
    qDebug() << "Undefined row" << index.row();
    return QVariant();
  }
  QList<QString> keys= mapFlags.keys();
  QString key=keys.at(index.row());
  switch(role){
    case KeyRole:
      return key;
    case ValueRole:
      return mapFlags[key];
    case InProgressRole:
      return inProgressFlags.contains(key);
  }
  return QVariant();
}

QHash<int, QByteArray> StyleFlagsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[KeyRole]        = "key";
  roles[ValueRole]      = "value";
  roles[InProgressRole] = "inProgress";

  return roles;
}

Qt::ItemFlags StyleFlagsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void StyleFlagsModel::onFlagSet(QString key, bool value)
{
  beginResetModel();
  qDebug() << "Setup style flag" << key << "to" << value;
  inProgressFlags.remove(key);
  endResetModel();
}

Q_INVOKABLE void StyleFlagsModel::setFlag(const QString &key, bool value)
{
  beginResetModel();
  qDebug() << "Request style flag" << key << "to" << value;
  inProgressFlags.insert(key);
  mapFlags[key]=value;
  endResetModel();
  emit setFlagRequest(key, value);

  std::unordered_map<std::string,bool> flags;
  for (const auto &key:mapFlags.keys()){
    flags[key.toStdString()]=mapFlags[key];
  }
  SettingsRef settings=OSMScoutQt::GetInstance().GetSettings();
  settings->SetStyleSheetFlags(flags);
}
