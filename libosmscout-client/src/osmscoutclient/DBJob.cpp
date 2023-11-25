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
#include <osmscoutclient/DBJob.h>

namespace osmscout {

DBJob::DBJob():
  threadId(std::this_thread::get_id())
{
}

DBJob::~DBJob()
{
  assert(threadId==std::this_thread::get_id());
  Close();
}

void DBJob::Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
                const std::list<DBInstanceRef> &databases,
                std::shared_lock<std::shared_mutex> &&locker)
{
  assert(threadId==std::this_thread::get_id());
  this->basemapDatabase=basemapDatabase;
  this->databases=databases;
  this->locker=std::move(locker);
}

void DBJob::Close()
{
  if (!locker.owns_lock()){
    return;
  }
  assert(threadId==std::this_thread::get_id());
  locker.unlock();
  databases.clear();
}

}
