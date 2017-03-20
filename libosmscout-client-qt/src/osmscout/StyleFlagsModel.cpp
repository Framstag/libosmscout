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

StyleFlagsModel::StyleFlagsModel():
  QAbstractListModel()
{
  DBThread* dbThread = DBThread::GetInstance();
  connect(dbThread,SIGNAL(stylesheetFilenameChanged()),
          this,SLOT(onStyleChanged()),
          Qt::QueuedConnection);
  onStyleChanged();
}

StyleFlagsModel::~StyleFlagsModel()
{
}

void StyleFlagsModel::onStyleChanged()
{
  beginResetModel();
  DBThread* dbThread = DBThread::GetInstance();
  mapFlags=dbThread->GetStyleFlags();
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
  }
  return QVariant();
}

QHash<int, QByteArray> StyleFlagsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[KeyRole]   = "key";
  roles[ValueRole] = "value";

  return roles;
}

Qt::ItemFlags StyleFlagsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
      return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

Q_INVOKABLE void StyleFlagsModel::setFlag(const QString &key, bool value)
{
  qDebug() << "Setup style flag" << key << "to" << value;
  DBThread* dbThread = DBThread::GetInstance();
  dbThread->SetStyleFlag(key, value);
  QMap<QString,bool> updated=dbThread->GetStyleFlags();
  
  std::unordered_map<std::string,bool> flags;
  for (const auto &key:updated.keys()){
    flags[key.toStdString()]=updated[key];
  }
  SettingsRef settings=DBThread::GetInstance()->GetSettings();
  settings->SetStyleSheetFlags(flags);
}
