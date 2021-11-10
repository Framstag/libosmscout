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

#include <osmscoutclientqt/DBInstance.h>

#include <osmscout/util/Logger.h>

namespace osmscout {

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

    osmscout::log.Info()<< "Create new style with " << stylesheetFilename.toStdString();
  }
  else {
    std::list<std::string> errorsStrings=newStyleConfig->GetErrors();

    for(const auto& errorString : errorsStrings) {
      StyleError err(QString::fromStdString(errorString));
      qWarning() << "Style error:" << err.GetDescription();
      errors.append(err);
    }

    styleConfig=nullptr;

    return false;
  }

  return true;
}

osmscout::MapPainterQt* DBInstance::GetPainter()
{
  QMutexLocker locker(&mutex);
  if (!styleConfig)
    return nullptr;

  if (!painterHolder.contains(QThread::currentThread())){
    painterHolder[QThread::currentThread()]=new osmscout::MapPainterQt(styleConfig);
    connect(QThread::currentThread(), &QThread::finished,
            this, &DBInstance::onThreadFinished);
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

  qDeleteAll(painterHolder);
  painterHolder.clear();

  // release map service, its threads may still use database
  // threads are stopped and joined in MapService destructor
  if (mapService && mapService.use_count() > 1){
    // if DBInstance is not exclusive owner, threads may hit closed data file and trigger assert!
    log.Warn() << "Map service for " << path.toStdString() << " is used on multiple places";
  }
  mapService.reset();

  locationService.reset();
  locationDescriptionService.reset();
  styleConfig.reset();

  if (database && database->IsOpen()) {
    database->Close();
  }
  database.reset();
}
}
