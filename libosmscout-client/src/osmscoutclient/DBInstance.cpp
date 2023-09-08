/*
  This source is part of the libosmscout library
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

#include <osmscoutclient/DBInstance.h>

#include <osmscoutmap/StyleError.h>

#include <osmscout/log/Logger.h>

namespace osmscout {

bool DBInstance::LoadStyle(const std::string &stylesheetFilename,
                           std::unordered_map<std::string,bool> stylesheetFlags,
                           std::list<StyleError> &errors)
{
  std::scoped_lock lock(mutex);

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

  if (newStyleConfig->Load(stylesheetFilename, nullptr, false)) {
    // Tear down
    painterHolder.clear();

    // Recreate
    styleConfig=newStyleConfig;

    log.Info()<< "Created new style with " << stylesheetFilename;
  }
  else {
    errors=newStyleConfig->GetErrors();

    for(const auto& err : errors) {
      log.Warn() << "Style error:" << err.GetDescription();
    }

    styleConfig=nullptr;

    return false;
  }

  return true;
}

void DBInstance::OnThreadFinished(const std::thread::id &id)
{
  std::scoped_lock lock(mutex);
  if (auto it=painterHolder.find(id);
      it!=painterHolder.end()){
    painterHolder.erase(it);
  }
}

void DBInstance::Close()
{
  std::scoped_lock lock(mutex);

  painterHolder.clear();

  // release map service, its threads may still use db
  // threads are stopped and joined in MapService destructor
  if (mapService && mapService.use_count() > 1){
    // if DBInstance is not exclusive owner, threads may hit closed data file and trigger assert!
    log.Warn() << "Map service for " << path << " is used on multiple places";
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
