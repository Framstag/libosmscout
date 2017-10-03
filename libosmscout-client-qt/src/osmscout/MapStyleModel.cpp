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


#include <osmscout/MapStyleModel.h>
#include <osmscout/DBThread.h>
#include <osmscout/OSMScoutQt.h>

MapStyleModel::MapStyleModel():
  QAbstractListModel()
{
  styleModule=OSMScoutQt::GetInstance().MakeStyleModule();
  connect(styleModule,SIGNAL(stylesheetFilenameChanged()),
          this,SIGNAL(styleChanged()),
          Qt::QueuedConnection);
  connect(this,SIGNAL(loadStyleRequested(QString,std::unordered_map<std::string,bool>)),
          styleModule,SLOT(loadStyle(QString,std::unordered_map<std::string,bool>)),
          Qt::QueuedConnection);

  SettingsRef settings=OSMScoutQt::GetInstance().GetSettings();

  QDirIterator dirIt(settings->GetStyleSheetDirectory(), QDirIterator::FollowSymlinks);
  while (dirIt.hasNext()) {
    dirIt.next();
    QFileInfo fInfo(dirIt.filePath());
    if (fInfo.suffix()=="oss"){
      stylesheets << fInfo;
    }
  }
}

MapStyleModel::~MapStyleModel()
{
  if (styleModule!=NULL){
    styleModule->deleteLater();
    styleModule=NULL;
  }
}

QString MapStyleModel::getStyle() const
{
  QFileInfo fileInfo(OSMScoutQt::GetInstance().GetDBThread()->GetStylesheetFilename());
  return fileInfo.fileName();
}

void MapStyleModel::setStyle(const QString &styleFile)
{
  SettingsRef settings=OSMScoutQt::GetInstance().GetSettings();

  settings->SetStyleSheetFile(styleFile);

  emit loadStyleRequested(settings->GetStyleSheetAbsoluteFile(),
                          settings->GetStyleSheetFlags());
}

Q_INVOKABLE int MapStyleModel::indexOf(const QString &style) const
{
  int i=0;
  for (const auto &stylesheet:stylesheets){
    if (stylesheet.fileName()==style){
      return i;
    }
    i++;
  }
  return -1;
}

Q_INVOKABLE QString MapStyleModel::file(int row) const
{
  if(row < 0 || row >= stylesheets.size()) {
    qDebug() << "Undefined row" << row;
    return "";
  }
  return stylesheets.at(row).fileName();
}

QVariant MapStyleModel::data(const QModelIndex &index, int role) const
{
  if(index.row() < 0 || index.row() >= stylesheets.size()) {
    qDebug() << "Undefined row" << index.row();
    return QVariant();
  }
  QFileInfo file = stylesheets.at(index.row());
  switch(role){
    case NameRole:
      return file.baseName();
    case FileRole:
      return file.fileName();
    case PathRole:
      return file.absoluteFilePath();
  }
  return QVariant();
}

QHash<int, QByteArray> MapStyleModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[NameRole] = "name";
  roles[FileRole] = "file";
  roles[PathRole] = "path";

  return roles;
}

Qt::ItemFlags MapStyleModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

