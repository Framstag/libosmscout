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

#include <osmscoutmap/MapDataBudget.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <mutex>

#include <osmscout/log/Logger.h>
#include <osmscout/util/GeoBox.h>

#include <osmscoutmap/MapDataAccounting.h>

namespace osmscout {

  bool MapDataBudget::SetTotalBudget(size_t bytes)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    if (bytes==0) {
      log.Warn() << "Rejected a map data budget of zero bytes, reset the budget instead of setting it to zero";

      return false;
    }

    if (bytes<floorBytes) {
      log.Warn() << "Rejected a map data budget, it is below the floor of " << floorBytes << " bytes";

      return false;
    }

    totalBudgetBytes=bytes;

    return true;
  }

  void MapDataBudget::ResetBudget()
  {
    std::scoped_lock<std::mutex> guard(mutex);

    totalBudgetBytes=0;
  }

  bool MapDataBudget::IsBounded() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return totalBudgetBytes>0;
  }

  size_t MapDataBudget::GetTotalBudget() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return totalBudgetBytes;
  }

  size_t MapDataBudget::GetTotalBudgetWeight() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return MapDataAccounting::GetWeightForBytes(totalBudgetBytes);
  }

  bool MapDataBudget::SetFloor(size_t bytes)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    if (totalBudgetBytes>0 && bytes>totalBudgetBytes) {
      log.Warn() << "Rejected a map data cache floor, it is above the budget of " << totalBudgetBytes << " bytes";

      return false;
    }

    floorBytes=bytes;

    return true;
  }

  size_t MapDataBudget::GetFloor() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return floorBytes;
  }

  size_t MapDataBudget::GetFloorWeight() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return MapDataAccounting::GetWeightForBytes(floorBytes);
  }

  MapDataBudget::ContributorId MapDataBudget::AddContributor()
  {
    std::scoped_lock<std::mutex> guard(mutex);

    ContributorId                id=nextContributorId;

    nextContributorId++;

    Contributor contributor;

    contributor.lastRelevant=std::chrono::steady_clock::now();

    contributors[id]=contributor;

    return id;
  }

  void MapDataBudget::RemoveContributor(ContributorId id)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    contributors.erase(id);
  }

  // The parameters are not interchangeable although both are a size, hence the explicit suppression
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  void MapDataBudget::SetContributorWeight(ContributorId id,
                                           size_t weight)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      log.Warn() << "Ignored the usage of the unknown map data budget contributor " << id;

      return;
    }

    entry->second.weight=weight;
  }

  size_t MapDataBudget::GetContributorWeight(ContributorId id) const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      return 0;
    }

    return entry->second.weight;
  }

  size_t MapDataBudget::GetContributorCount() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return contributors.size();
  }

  size_t MapDataBudget::GetUsageWeight() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    size_t                       weight=0;

    for (const auto& entry : contributors) {
      weight+=entry.second.weight;
    }

    return weight;
  }

  size_t MapDataBudget::GetUsage() const
  {
    return MapDataAccounting::GetBytes(GetUsageWeight());
  }

  bool MapDataBudget::IsExceeded() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    if (totalBudgetBytes==0) {
      return false;
    }

    size_t weight=0;

    for (const auto& entry : contributors) {
      weight+=entry.second.weight;
    }

    return weight>MapDataAccounting::GetWeightForBytes(totalBudgetBytes);
  }

  void MapDataBudget::SetContributorExtent(ContributorId id,
                                           const GeoBox& extent)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      log.Warn() << "Ignored the extent of the unknown map data budget contributor " << id;

      return;
    }

    entry->second.extent=extent;
    entry->second.hasExtent=true;
  }

  void MapDataBudget::ReportView(const GeoBox& viewBox,
                                 TimePoint now)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    for (auto& entry : contributors) {
      bool relevant=entry.second.hasExtent && entry.second.extent.Intersects(viewBox);

      if (relevant) {
        entry.second.lastRelevant=now;
        entry.second.released=false;
      }

      if (entry.second.relevant==relevant) {
        continue;
      }

      entry.second.relevant=relevant;

      // The shares that are in effect stay in effect until the new set has been stable long enough, so
      // a cache that becomes relevant again inside the settling period keeps its share
      lastRelevanceChange=now;
    }
  }

  void MapDataBudget::SetIdleTime(std::chrono::milliseconds idleTime)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    this->idleTime=idleTime;
  }

  std::chrono::milliseconds MapDataBudget::GetIdleTime() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return idleTime;
  }

  bool MapDataBudget::IsContributorIdle(ContributorId id,
                                        TimePoint now) const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      return false;
    }

    if (entry->second.relevant || entry->second.released) {
      return false;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(now-entry->second.lastRelevant)>=idleTime;
  }

  bool MapDataBudget::IsContributorRelevant(ContributorId id) const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      return false;
    }

    return entry->second.relevant;
  }

  size_t MapDataBudget::GetRelevantContributorCount() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    size_t                       count=0;

    for (const auto& entry : contributors) {
      if (entry.second.relevant) {
        count++;
      }
    }

    return count;
  }

  void MapDataBudget::SetSettlingTime(std::chrono::milliseconds settlingTime)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    this->settlingTime=settlingTime;
  }

  std::chrono::milliseconds MapDataBudget::GetSettlingTime() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return settlingTime;
  }

  bool MapDataBudget::DistributeIfStable(TimePoint now)
  {
    std::scoped_lock<std::mutex> guard(mutex);

    if (totalBudgetBytes==0 || contributors.empty()) {
      return false;
    }

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastRelevanceChange)<settlingTime) {
      return false;
    }

    size_t budgetWeight=MapDataAccounting::GetWeightForBytes(totalBudgetBytes);
    size_t floorWeight=MapDataAccounting::GetWeightForBytes(floorBytes);

    // The floor of every registered cache is reserved in the budget. A budget that cannot hold the
    // floor of every cache lowers the effective floor rather than being exceeded.
    size_t effectiveFloor=std::min(floorWeight,
                                   budgetWeight/contributors.size());
    size_t reservedWeight=effectiveFloor*contributors.size();
    size_t freeWeight=budgetWeight>reservedWeight ? budgetWeight-reservedWeight : 0;

    size_t relevantCount=0;

    for (const auto& entry : contributors) {
      if (entry.second.relevant) {
        relevantCount++;
      }
    }

    size_t shareWeight=effectiveFloor+(relevantCount>0 ? freeWeight/relevantCount : 0);
    bool   sharesChanged=false;

    for (auto& entry : contributors) {
      size_t newShareWeight=entry.second.relevant ? shareWeight : effectiveFloor;

      if (entry.second.shareWeight!=newShareWeight) {
        entry.second.shareWeight=newShareWeight;
        sharesChanged=true;
      }
    }

    return sharesChanged;
  }

  size_t MapDataBudget::GetShareWeight(ContributorId id) const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    auto                         entry=contributors.find(id);

    if (entry==contributors.end()) {
      return 0;
    }

    return entry->second.shareWeight;
  }

  size_t MapDataBudget::GetDistributedWeight() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    size_t                       weight=0;

    for (const auto& entry : contributors) {
      weight+=entry.second.shareWeight;
    }

    return weight;
  }

  bool MapDataBudget::IsDistributed() const
  {
    std::scoped_lock<std::mutex> guard(mutex);

    return std::ranges::any_of(contributors,
                               [](const auto& entry) {
                                 return entry.second.shareWeight>0;
                               });
  }
}
