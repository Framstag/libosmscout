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

#include <osmscoutclientqt/StyleModule.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <iostream>

namespace osmscout {
StyleModule::StyleModule(QThread *thread,DBThreadRef dbThread):
    QObject(),
    thread(thread),
    dbThread(dbThread)
{
  dbThread->databaseLoadFinished.Connect(dbLoadedSlot);
  dbThread->stylesheetFilenameChanged.Connect(stylesheetFilenameChangeSlot);

  connect(this, &StyleModule::stylesheetFilenameChanged,
          this, &StyleModule::onStyleChanged,
          Qt::QueuedConnection);
}

StyleModule::~StyleModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=nullptr){
    thread->quit();
  }
  *alive=false;
}

void StyleModule::loadStyle(QString stylesheetFilename,
                            std::unordered_map<std::string,bool> stylesheetFlags)
{
  dbThread->LoadStyle(stylesheetFilename.toStdString(),
                      stylesheetFlags);
}

void StyleModule::onStyleFlagsRequested(){
  onStyleChanged();
}

void StyleModule::onStyleChanged()
{
  QMap<QString, bool> qMap;
  auto stdMap=dbThread->GetStyleFlags();
  for (const auto &p: stdMap) {
    qMap[QString::fromStdString(p.first)] = p.second;
  }

  emit styleFlagsChanged(qMap);
}

void StyleModule::onSetFlagRequest(QString key, bool value)
{
  qDebug() << "Setting flag" << key << "to" << value;
  auto thisAlive=alive;
  dbThread->SetStyleFlag(key.toStdString(), value)
    .OnComplete([this, key, value, thisAlive](const bool &){
      if (*thisAlive) { // avoid to call slot with deleted object
        emit flagSet(key, value);
      }
      qDebug() << "Flag" << key << "setup done (" << value << ")";
    });
}
}
