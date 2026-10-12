#ifndef OSMSCOUT_MAP_MAPDATABUDGET_H
#define OSMSCOUT_MAP_MAPDATABUDGET_H

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

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/GeoBox.h>

#include <osmscoutmap/MapDataAccounting.h>
#include <osmscoutmap/MapImportExport.h>

namespace osmscout {

  /**
   * \ingroup MapDataBudget
   *
   * The memory budget shared by the map data caches of all open databases.
   *
   * A cache reports its accounted content as a weight (see MapDataAccounting) and is known to the
   * budget under a contributor id. The budget adds the weights of its contributors to the current
   * usage and compares it to the configured total.
   *
   * The budget is optional: a budget that was never set (or that has been reset) does not bound
   * anything and only reports the usage of its contributors.
   *
   * Which contributor holds how much of the budget follows the relevance of the cache to the current
   * view: the caches of the view the user is looking at share the budget, a cache that is not part of
   * the view is reduced to the floor. The distribution is not applied on every change of the view but
   * only once the set of relevant caches has been stable for the settling period, so that panning
   * along the border of two databases does not resize the caches on every frame.
   *
   * The budget is thread-safe: the map data caches of different databases are filled by different
   * worker threads, and a client may read the usage from another thread.
   */
  class OSMSCOUT_MAP_API MapDataBudget CLASS_FINAL
  {
  public:
    /**
     * Handle a cache is known under in the budget
     */
    using ContributorId=size_t;

    /**
     * Point in time the relevance reports and the settling are measured with
     */
    using TimePoint=std::chrono::steady_clock::time_point;

  private:
    /**
     * The state of one registered cache
     */
    struct Contributor CLASS_FINAL
    {
      bool      hasExtent=false;   //!< True if the geographic extent of the cache is known
      GeoBox    extent;            //!< Geographic extent of the data of the cache
      bool      relevant=false;    //!< True if the cache is part of the current view
      TimePoint lastRelevant;      //!< Time the cache was part of the view the last time
      bool      released=false;    //!< True if the content of the cache was released for idleness
      size_t    weight=0;          //!< Accounted weight the cache reported
      size_t    shareWeight=0;     //!< Accounted weight the cache may hold
    };

  private:
    mutable std::mutex                  mutex;         //!< Guards all members of the budget

    size_t                              totalBudgetBytes=0; //!< Total budget in bytes, zero if unbounded
    size_t                              floorBytes=0;  //!< Bytes a single cache keeps while out of view
    std::chrono::milliseconds           settlingTime=defaultSettlingTime; //!< Time the relevant set has to stay stable
    std::chrono::milliseconds           idleTime=defaultIdleTime;    //!< Time a cache may stay out of view before its content is released

    ContributorId                       nextContributorId=1;
    std::map<ContributorId,Contributor> contributors;   //!< Registered caches
    TimePoint                           lastRelevanceChange=std::chrono::steady_clock::now(); //!< When the relevant set changed last

  public:
    /**
     * Time the set of relevant caches has to stay stable before a distribution is applied
     */
    static constexpr std::chrono::milliseconds defaultSettlingTime=std::chrono::seconds(5);

    /**
     * Time a cache may stay out of view before the content of the cache is released
     */
    static constexpr std::chrono::milliseconds defaultIdleTime=std::chrono::minutes(10);

  public:
    MapDataBudget() = default;
    ~MapDataBudget() = default;

    MapDataBudget(const MapDataBudget&) = delete;
    MapDataBudget(MapDataBudget &&) = delete;

    MapDataBudget& operator=(const MapDataBudget&) = delete;
    MapDataBudget& operator=(MapDataBudget&&) = delete;

    /**
     * Set the total budget in bytes and return 'true' if it was accepted.
     *
     * A non-positive budget is not a bound and is rejected: a client that does not want a bound
     * resets the budget or simply never sets one. A non-zero budget below the configured floor is
     * rejected as well, because the floor of a single cache has to fit into the total.
     */
    bool SetTotalBudget(size_t bytes);

    /**
     * Drop the bound, keeping the reported usage. The budget acts as if no budget was ever set.
     */
    void ResetBudget();

    /**
     * Return 'true' if a bound is configured
     */
    bool IsBounded() const;

    /**
     * Return the total budget in bytes, zero if no bound is configured
     */
    size_t GetTotalBudget() const;

    /**
     * Return the total budget in weight units, the unit the accounted content is compared in. A
     * non-zero number of bytes accounts as at least one weight unit.
     */
    size_t GetTotalBudgetWeight() const;

    /**
     * Set the number of bytes a single cache keeps while it is not relevant to the view and return
     * 'true' if it was accepted. A non-zero floor above the total budget is rejected, because no
     * cache could then stay within the total.
     */
    bool SetFloor(size_t bytes);

    /**
     * Return the floor in bytes, zero if a cache may be emptied
     */
    size_t GetFloor() const;

    /**
     * Return the floor in weight units
     */
    size_t GetFloorWeight() const;

    /**
     * Register a cache and return the id it is known under
     */
    ContributorId AddContributor();

    /**
     * Deregister a cache and drop the usage it reported
     */
    void RemoveContributor(ContributorId id);

    /**
     * Replace the accounted weight the contributor reported
     */
    void SetContributorWeight(ContributorId id,
                              size_t weight);

    /**
     * Return the accounted weight the contributor reported, zero if it is not registered
     */
    size_t GetContributorWeight(ContributorId id) const;

    /**
     * Return the number of registered caches
     */
    size_t GetContributorCount() const;

    /**
     * Return the accounted weight of all registered caches
     */
    size_t GetUsageWeight() const;

    /**
     * Return the number of bytes the accounted weight of all registered caches stands for
     */
    size_t GetUsage() const;

    /**
     * Return 'true' if the accounted weight of all registered caches exceeds the total budget. A
     * budget without a bound is never exceeded.
     */
    bool IsExceeded() const;

    /**
     * Tell the budget the geographic extent of the data of the cache. A cache without a known extent
     * is never part of the view.
     */
    void SetContributorExtent(ContributorId id,
                              const GeoBox& extent);

    /**
     * Report the area the user currently looks at. Every registered cache whose extent covers that
     * area becomes part of the relevant set, every other cache leaves it.
     *
     * A changed relevant set restarts the settling period of the budget; the shares that were
     * distributed before stay in effect until the set of relevant caches has been stable again.
     */
    void ReportView(const GeoBox& viewBox,
                    TimePoint now);

    /**
     * Return 'true' if the cache is part of the current view
     */
    bool IsContributorRelevant(ContributorId id) const;

    /**
     * Return the number of registered caches that are part of the current view
     */
    size_t GetRelevantContributorCount() const;

    /**
     * Set the time the set of relevant caches has to stay stable before a distribution is applied
     */
    void SetSettlingTime(std::chrono::milliseconds settlingTime);

    /**
     * Return the time the set of relevant caches has to stay stable before a distribution is applied
     */
    std::chrono::milliseconds GetSettlingTime() const;

    /**
     * Distribute the budget over the registered caches if the set of relevant caches has been stable
     * for the settling period, and return 'true' if the shares changed by doing so.
     *
     * The relevant caches share the budget, every other cache is reduced to the floor. The floor of
     * every cache is reserved in the budget, and a budget that cannot hold the floor of every
     * registered cache lowers the effective floor instead of being exceeded.
     */
    bool DistributeIfStable(TimePoint now);

    /**
     * Return the accounted weight the cache may hold, zero if no distribution was applied yet
     */
    size_t GetShareWeight(ContributorId id) const;

    /**
     * Return the sum of the shares of all registered caches: the accounted weight the caches may hold
     * together. It is at or below the total budget.
     */
    size_t GetDistributedWeight() const;

    /**
     * Return 'true' if a distribution was applied
     */
    bool IsDistributed() const;

    /**
     * Set the time a cache may stay out of view before the content of the cache is released
     */
    void SetIdleTime(std::chrono::milliseconds idleTime);

    /**
     * Return the time a cache may stay out of view before the content of the cache is released
     */
    std::chrono::milliseconds GetIdleTime() const;

    /**
     * Return 'true' if the cache is out of view for longer than the idle time and its content should
     * be released. A cache whose content was released already is not reported again until it became
     * part of the view again.
     */
    bool IsContributorIdle(ContributorId id,
                           TimePoint now) const;
  };

  /**
   * \ingroup MapDataBudget
   *
   * Reference counted reference to a MapDataBudget instance
   */
  using MapDataBudgetRef = std::shared_ptr<MapDataBudget>;
}

#endif
