#ifndef OSMSCOUT_CLIENT_DBJOB_H
#define OSMSCOUT_CLIENT_DBJOB_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017 Lukáš Karas

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/DBInstance.h>

#include <osmscout/db/BasemapDatabase.h>

#include <thread>
#include <list>
#include <shared_mutex>

namespace osmscout {

/**
 * \ingroup ClientAPI
 */
class OSMSCOUT_CLIENT_API DBJob {

  protected:
  osmscout::BasemapDatabaseRef basemapDatabase; //!< Optional reference to the basemap db
  std::list<DBInstanceRef>     databases;       //!< borrowed databases
  std::thread::id              threadId;        //!< job thread

  private:
  std::shared_lock<std::shared_mutex> locker;   //!< db locker

  public:
  DBJob();
  DBJob(const DBJob&) = delete;
  DBJob(DBJob&&) = delete;
  DBJob& operator=(const DBJob&) = delete;
  DBJob& operator=(DBJob&&) = delete;
  virtual ~DBJob();

  virtual void Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
  const std::list<DBInstanceRef> &databases,
  std::shared_lock<std::shared_mutex> &&locker);
  virtual void Close();
};

}

#endif //OSMSCOUT_CLIENT_DBJOB_H
