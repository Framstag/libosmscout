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

#include <osmscout/StyleModule.h>
#include <osmscout/OSMScoutQt.h>
#include <iostream>

StyleModule::StyleModule(QThread *thread,DBThreadRef dbThread):
    QObject(),
    thread(thread),
    dbThread(dbThread)
{

  connect(dbThread.get(), SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)),
          this, SIGNAL(initialisationFinished(const DatabaseLoadedResponse&)));
  connect(dbThread.get(), SIGNAL(stylesheetFilenameChanged()),
          this, SIGNAL(stylesheetFilenameChanged()));
  connect(dbThread.get(),SIGNAL(stylesheetFilenameChanged()),
          this,SLOT(onStyleChanged()),
          Qt::QueuedConnection);
}

StyleModule::~StyleModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=NULL){
    thread->quit();
  }
}

void StyleModule::loadStyle(QString stylesheetFilename,
                            std::unordered_map<std::string,bool> stylesheetFlags)
{
  dbThread->LoadStyle(stylesheetFilename,
                      stylesheetFlags);
}

void StyleModule::onStyleFlagsRequested(){
  onStyleChanged();
}

void StyleModule::onStyleChanged()
{
  emit styleFlagsChanged(dbThread->GetStyleFlags());
}

void StyleModule::onSetFlagRequest(QString key, bool value)
{
  dbThread->SetStyleFlag(key, value);
  emit flagSet(key, value);
}