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

#ifndef MAPMANAGER_H
#define	MAPMANAGER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDir>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapManager: public QObject
{
  Q_OBJECT
  
private:
  QStringList databaseLookupDirs;
  QList<QDir> databaseDirectories;

public slots:
  void lookupDatabases();
  
signals:
  void databaseListChanged(QList<QDir> databaseDirectories);
  
public:
  MapManager(QStringList databaseLookupDirs);
  
  inline ~MapManager(){};
};

#endif	/* MAPMANAGER_H */

