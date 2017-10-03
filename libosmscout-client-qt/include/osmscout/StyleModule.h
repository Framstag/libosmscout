#ifndef OSMSCOUT_CLIENT_QT_STYLEMODULE_H
#define OSMSCOUT_CLIENT_QT_STYLEMODULE_H

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

#include <QObject>
#include <QThread>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API StyleModule:public QObject {
  Q_OBJECT

private:
  QMutex           mutex;
  QThread          *thread;
  DBThreadRef      dbThread;

signals:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void stylesheetFilenameChanged();
  void styleFlagsChanged(QMap<QString,bool>);
  void flagSet(QString key, bool value);

public slots:
  void loadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags);
  void onStyleChanged();
  void onStyleFlagsRequested();
  void onSetFlagRequest(QString key, bool value);

public:
  StyleModule(QThread *thread,DBThreadRef dbThread);
  virtual ~StyleModule();
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<StyleModule> StyleModuleRef;

#endif //OSMSCOUT_CLIENT_QT_STYLEMODULE_H
