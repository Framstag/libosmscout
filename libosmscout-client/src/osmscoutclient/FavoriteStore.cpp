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

#include <osmscoutclient/FavoriteStore.h>

namespace osmscout {

FavoriteStore::~FavoriteStore()
{
  Shutdown();
}

bool FavoriteStore::ReplaceByPath(const std::string &filePath)
{
  std::scoped_lock lock(mutex_);

  service_ = std::make_unique<FavoriteLocationService>(filePath);

  return true;
}

bool FavoriteStore::ReplaceAndSave(const std::string &filePath,
                                   const std::vector<FavLocationGroup> &groups)
{
  std::scoped_lock lock(mutex_);

  service_ = std::make_unique<FavoriteLocationService>(filePath);

  // The constructor loads whatever the file held; the caller's snapshot is the
  // complete new content, so start from an empty store.
  service_->ClearAll();

  for (const auto &group : groups) {
    service_->AddGroup(group.name);

    // Group attributes go through the service's own setters so they are stored
    // the way the service expects (currently only the color is representable).
    for (const auto &[key, value] : group.attributes) {
      if (key == "color") {
        service_->SetGroupColor(group.name, value);
      }
    }

    for (const auto &fav : group.favorites) {
      service_->AddFavorite(group.name, fav);
    }
  }

  return service_->Save();
}

bool FavoriteStore::HasStore() const
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr;
}

void FavoriteStore::Shutdown()
{
  std::scoped_lock lock(mutex_);

  service_.reset();
}

std::vector<FavLocationGroup> FavoriteStore::GetGroups() const
{
  std::scoped_lock lock(mutex_);

  if (service_ == nullptr) {
    return {};
  }

  return service_->GetGroups();
}

bool FavoriteStore::AddGroup(const std::string &name)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->AddGroup(name);
}

bool FavoriteStore::DeleteGroup(const std::string &name)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->DeleteGroup(name);
}

bool FavoriteStore::RenameGroup(const std::string &oldName,
                                const std::string &newName)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->RenameGroup(oldName, newName);
}

bool FavoriteStore::AddFavorite(const std::string &groupName,
                                const FavLocation &fav)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->AddFavorite(groupName, fav);
}

bool FavoriteStore::DeleteFavorite(const std::string &groupName,
                                   const std::string &favName)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->DeleteFavorite(groupName, favName);
}

bool FavoriteStore::RenameFavorite(const std::string &groupName,
                                   const std::string &oldName,
                                   const std::string &newName)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->RenameFavorite(groupName, oldName, newName);
}

bool FavoriteStore::MoveFavorite(const std::string &groupName,
                                 const std::string &favName,
                                 size_t newIndex)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->MoveFavorite(groupName, favName, newIndex);
}

bool FavoriteStore::SetStarred(const std::string &groupName,
                               const std::string &favName,
                               bool starred)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->SetStarred(groupName, favName, starred);
}

bool FavoriteStore::IsStarred(const std::string &groupName,
                              const std::string &favName) const
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->IsStarred(groupName, favName);
}

bool FavoriteStore::SetGroupColor(const std::string &groupName,
                                  const std::string &color)
{
  std::scoped_lock lock(mutex_);

  return service_ != nullptr && service_->SetGroupColor(groupName, color);
}

std::string FavoriteStore::GetGroupColor(const std::string &groupName) const
{
  std::scoped_lock lock(mutex_);

  if (service_ == nullptr) {
    return {};
  }

  return service_->GetGroupColor(groupName);
}

}
