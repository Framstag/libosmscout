/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/db/BasemapDatabase.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  void BasemapDatabaseParameter::SetDataMMap(bool mmap)
  {
    dataMMap=mmap;
  }

  bool BasemapDatabaseParameter::GetDataMMap() const
  {
    return dataMMap;
  }

  BasemapDatabase::BasemapDatabase(const BasemapDatabaseParameter& parameter)
  : parameter(parameter),
    isOpen(false)
  {
    log.Debug() << "BasemapDatabase::BasemapDatabase()";
  }

  BasemapDatabase::~BasemapDatabase()
  {
    log.Debug() << "BasemapDatabase::~BasemapDatabase()";

    if (IsOpen()) {
      Close();
    }
  }

  bool BasemapDatabase::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    isOpen=true;

    return true;
  }

  bool BasemapDatabase::IsOpen() const
  {
    return isOpen;
  }

  void BasemapDatabase::Close()
  {

    if (waterIndex) {
      waterIndex->Close();
      waterIndex=nullptr;
    }

    isOpen=false;
  }

  std::string BasemapDatabase::GetPath() const
  {
    return path;
  }

  WaterIndexRef BasemapDatabase::GetWaterIndex() const
  {
    std::scoped_lock<std::mutex> guard(waterIndexMutex);

    if (!IsOpen()) {
      return nullptr;
    }

    if (!waterIndex) {
      waterIndex=std::make_shared<WaterIndex>();

      StopClock timer;

      if (!waterIndex->Open(path, parameter.GetDataMMap())) {
        log.Error() << "Cannot load water index!";
        waterIndex=nullptr;

        return nullptr;
      }

      timer.Stop();

      log.Debug() << "Opening WaterIndex: " << timer.ResultString();
    }

    return waterIndex;
  }
}
