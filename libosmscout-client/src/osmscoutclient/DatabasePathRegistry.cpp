/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutclient/DatabasePathRegistry.h>

#include <algorithm>
#include <system_error>

namespace osmscout {

bool IsOpenableDatabaseDirectory(const std::filesystem::path &path) noexcept
{
  // The error-code overload never throws: a permission failure, an unmounted
  // path or any other filesystem error is a "no" for this question.
  std::error_code error;

  return std::filesystem::is_directory(path, error);
}

bool DatabasePathRegistry::Register(const std::filesystem::path &path)
{
  std::scoped_lock lock(mutex_);

  setChangeCount_++;

  auto it = std::find(paths_.begin(), paths_.end(), path);

  if (it == paths_.end()) {
    paths_.push_back(path);
  }

  // Registered either way: a path already in the set stays in it.
  return true;
}

bool DatabasePathRegistry::RegisterOpenable(const std::filesystem::path &path)
{
  // Decided before the lock is taken, so a slow filesystem cannot block another
  // opener or the database thread.
  if (!IsOpenableDatabaseDirectory(path)) {
    return false;
  }

  return Register(path);
}

DatabasePathRegistration DatabasePathRegistry::RegisterAll(const std::vector<std::filesystem::path> &paths)
{
  std::scoped_lock lock(mutex_);

  setChangeCount_++;

  DatabasePathRegistration result;

  result.registered.reserve(paths.size());

  for (const auto &path : paths) {
    auto it = std::find(paths_.begin(), paths_.end(), path);

    if (it == paths_.end()) {
      paths_.push_back(path);
      result.added++;
    }

    // Registered either way: a path already in the set stays in it.
    result.registered.push_back(true);
  }

  result.paths = paths_;

  return result;
}

std::vector<std::filesystem::path> DatabasePathRegistry::Snapshot() const
{
  std::scoped_lock lock(mutex_);

  return paths_;
}

size_t DatabasePathRegistry::Size() const
{
  std::scoped_lock lock(mutex_);

  return paths_.size();
}

size_t DatabasePathRegistry::SetChangeCount() const
{
  std::scoped_lock lock(mutex_);

  return setChangeCount_;
}

void DatabasePathRegistry::Clear()
{
  std::scoped_lock lock(mutex_);

  paths_.clear();
}

}
