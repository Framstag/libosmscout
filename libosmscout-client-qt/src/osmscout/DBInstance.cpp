/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017  Lukáš Karas

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

#include <osmscout/DBInstance.h>

QBreaker::QBreaker()
  : osmscout::Breaker(),
    aborted(false)
{
}

void QBreaker::Break()
{
  QMutexLocker locker(&mutex);

  aborted=true;
}

bool QBreaker::IsAborted() const
{
  QMutexLocker locker(&mutex);

  return aborted;
}

void QBreaker::Reset()
{
  QMutexLocker locker(&mutex);

  aborted=false;
}

StyleError::StyleError(QString msg){
    QRegExp rx("(\\d+),(\\d+) (Symbol|Error|Warning|Exception):(.*)");
    if(rx.exactMatch(msg)){
        line = rx.cap(1).toInt();
        column = rx.cap(2).toInt();
        if(rx.cap(3) == "Symbol"){
            type = Symbol;
        } else if(rx.cap(3) == "Error"){
            type = Error;
        } else if(rx.cap(3) == "Warning"){
            type = Warning;
        } else {
            type = Exception;
        }
        text = rx.cap(4);
    }
}

QString StyleError::GetTypeName() const
{
    switch(type){
    case Symbol:
        return QString("symbol");
        break;
    case Error:
        return QString("error");
        break;
    case Warning:
        return QString("warning");
        break;
    case Exception:
        return QString("exception");
        break;
    default:
      return QString("???");
    }
}

bool DBInstance::LoadStyle(QString stylesheetFilename,
                           std::unordered_map<std::string,bool> stylesheetFlags,
                           QList<StyleError> &errors)
{
  QMutexLocker locker(&mutex);

  if (!database->IsOpen()) {
    return false;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return false;
  }

  // new map style may require more data types. when tile is marked as "completed"
  // such data types are never loaded into these tiles
  // so we mark them as "incomplete" to make sure that all types for new stylesheet are loaded
  mapService->InvalidateTileCache();
  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  for (const auto& flag : stylesheetFlags) {
    newStyleConfig->AddFlag(flag.first,flag.second);
  }

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    qDeleteAll(painterHolder);
    painterHolder.clear();

    // Recreate
    styleConfig=newStyleConfig;

    std::cout << "Create new style with " << stylesheetFilename.toStdString() << std::endl;
  }
  else {
    std::list<std::string> errorsStrings=newStyleConfig->GetErrors();

    for(const auto& errorString : errorsStrings) {
      StyleError err(QString::fromStdString(errorString));
      qWarning() << "Style error:" << err.GetDescription();
      errors.append(err);
    }

    styleConfig=NULL;

    return false;
  }

  return true;
}


bool DBInstance::AssureRouter(osmscout::Vehicle /*vehicle*/,
                              osmscout::RouterParameter routerParameter)
{
  QMutexLocker locker(&mutex);
  if (!database->IsOpen()) {
    return false;
  }

  if (!router/* ||
      (router && router->GetVehicle()!=vehicle)*/) {
    if (router) {
      if (router->IsOpen()) {
        router->Close();
      }
      router=NULL;
    }

    router=std::make_shared<osmscout::RoutingService>(database,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);

    if (!router->Open()) {
      return false;
    }
  }

  return true;
}

osmscout::MapPainterQt* DBInstance::GetPainter()
{
  QMutexLocker locker(&mutex);
  if (!painterHolder.contains(QThread::currentThread()) && styleConfig){
    painterHolder[QThread::currentThread()]=new osmscout::MapPainterQt(styleConfig);
    connect(QThread::currentThread(),SIGNAL(finished()),
            this,SLOT(onThreadFinished()));
  }
  return painterHolder[QThread::currentThread()];
}

void DBInstance::onThreadFinished()
{
  QMutexLocker locker(&mutex);
  if (painterHolder.contains(QThread::currentThread())){
    delete painterHolder[QThread::currentThread()];
    painterHolder.remove(QThread::currentThread());
  }
}

void DBInstance::close()
{
  QMutexLocker locker(&mutex);
  if (router && router->IsOpen()) {
    router->Close();
  }

  qDeleteAll(painterHolder);
  painterHolder.clear();

  if (callbackId){
    mapService->DeregisterTileStateCallback(callbackId);
  }
  callbackId = 0;
  if (database->IsOpen()) {
    database->Close();
  }
}
