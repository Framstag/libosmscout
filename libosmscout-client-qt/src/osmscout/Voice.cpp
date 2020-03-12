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

#include <osmscout/AvailableVoicesModel.h>
#include <osmscout/PersistentCookieJar.h>
#include <osmscout/OSMScoutQt.h>

namespace osmscout {

AvailableVoice::AvailableVoice(const VoiceProvider &provider,
                               const QString &lang,
                               const QString &gender,
                               const QString &name,
                               const QString &license,
                               const QString &directory,
                               const QString &author,
                               const QString &description) :
    valid(true), provider(provider), lang(lang), gender(gender), name(name), license(license),
    directory(directory), author(author), description(description)
{
}

AvailableVoice::AvailableVoice(const AvailableVoice &o) :
    QObject(o.parent()), valid(o.valid), provider(o.provider), lang(o.lang), gender(o.gender),
    name(o.name), license(o.license), directory(o.directory),
    author(o.author), description(o.description)
{}

bool Voice::deleteVoice()
{
  valid=false;

  QStringList fileNames = files();
  fileNames << MapDownloadJob::FILE_METADATA;

  bool result=true;
  for (const auto &fileName: fileNames) {
    if(dir.exists(fileName)){
      result&=dir.remove(fileName);
    }
  }
  QDir parent=dir;
  parent.cdUp();
  result&=parent.rmdir(dir.dirName());
  if (result){
    qDebug() << "Removed voice" << dir.path();
  }else{
    qWarning() << "Failed to remove voice directory completely" << dir.path();
  }
  return result;
}

QStringList Voice::files()
{
  QStringList fileNames;
  fileNames << "After.ogg"
            << "AhExitLeft.ogg"
            << "AhExit.ogg"
            << "AhExitRight.ogg"
            << "AhFerry.ogg"
            << "AhKeepLeft.ogg"
            << "AhKeepRight.ogg"
            << "AhLeftTurn.ogg"
            << "AhRightTurn.ogg"
            << "AhUTurn.ogg"
            << "Arrive.ogg"
            << "AUTHORS.txt"
            << "BearLeft.ogg"
            << "BearRight.ogg"
            << "Depart.ogg"
            << "GpsFound.ogg"
            << "GpsLost.ogg"
            << "Charge.ogg"
            << "KeepLeft.ogg"
            << "KeepRight.ogg"
            << "LICENSE.txt"
            << "LnLeft.ogg"
            << "LnRight.ogg"
            << "Marble.ogg"
            << "Meters.ogg"
            << "MwEnter.ogg"
            << "MwExitLeft.ogg"
            << "MwExit.ogg"
            << "MwExitRight.ogg"
            << "RbBack.ogg"
            << "RbCross.ogg"
            << "RbExit1.ogg"
            << "RbExit2.ogg"
            << "RbExit3.ogg"
            << "RbExit4.ogg"
            << "RbExit5.ogg"
            << "RbExit6.ogg"
            << "RbLeft.ogg"
            << "RbRight.ogg"
            << "RoadEnd.ogg"
            << "RouteCalculated.ogg"
            << "RouteDeviated.ogg"
            << "SharpLeft.ogg"
            << "SharpRight.ogg"
            << "Straight.ogg"
            << "TakeFerry.ogg"
            << "Then.ogg"
            << "TryUTurn.ogg"
            << "TurnLeft.ogg"
            << "TurnRight.ogg"
            << "UTurn.ogg"
            << "Yards.ogg"
            << "100.ogg"
            << "2ndLeft.ogg"
            << "2ndRight.ogg"
            << "200.ogg"
            << "3rdLeft.ogg"
            << "3rdRight.ogg"
            << "300.ogg"
            << "400.ogg"
            << "50.ogg"
            << "500.ogg"
            << "600.ogg"
            << "700.ogg"
            << "80.ogg"
            << "800.ogg";

  return fileNames;
}

Voice::Voice(QDir dir):
    dir(dir)
{
  QStringList fileNames=Voice::files();
  osmscout::log.Debug() << "Checking voice files in directory " << dir.absolutePath().toStdString();
  valid=true;
  for (const auto &fileName: fileNames) {
    bool exists=dir.exists(fileName);
    if (!exists){
      osmscout::log.Debug() << "Missing mandatory file: " << fileName.toStdString();
    }
    valid &= exists;
  }
  if (!valid){
    osmscout::log.Warn() << "Can't use voice " << dir.absolutePath().toStdString() << ", some mandatory files are missing.";
  }

  // metadata
  if (dir.exists(MapDownloadJob::FILE_METADATA)){
    QFile jsonFile(dir.filePath(MapDownloadJob::FILE_METADATA));
    jsonFile.open(QFile::OpenModeFlag::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    QJsonObject metadataObject = doc.object();
    if (metadataObject.contains("lang") &&
        metadataObject.contains("gender") &&
        metadataObject.contains("name") &&
        metadataObject.contains("license") &&
        metadataObject.contains("author") &&
        metadataObject.contains("description")){

      lang = metadataObject["lang"].toString();
      gender = metadataObject["gender"].toString();
      name = metadataObject["name"].toString();
      license = metadataObject["license"].toString();
      author = metadataObject["author"].toString();
      description = metadataObject["description"].toString();

      metadata = true;
    }
  }
}

}
