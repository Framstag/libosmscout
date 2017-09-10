#ifndef OSMSCOUT_CACHE_H
#define OSMSCOUT_CACHE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/CoreFeatures.h>

#include <limits>
#include <list>
#include <unordered_map>
#include <vector>

#include <osmscout/system/Assert.h>

#include <osmscout/Types.h>

#include <osmscout/util/Logger.h>

namespace osmscout {

  /**
   * \ingroup Util
   * Generic FIFO cache implementation with O(n log n) semantic.
   *
   * Template parameter class K holds the key value (must be a numerical value),
   * parameter class V holds the data class that is to be cached,
   * and parameter IK holds the internal key value, must be an unsigned value,
   * default is PageId.
   *
   * * The cache is not threadsafe.
   * * It uses a std::vector<std::list>> as a hash table for data lookup
   * * It uses an std::list for implementing FIFO characteristics.
   *
   * Implementation details: If std::unordered_map ist available we use this
   * for fast detection (O(1)) if an object is already in the cast. If it is not
   * available, we use a vector as a hashtable via the key value with list
   * as entry type for hash code conflict handling.
   *
   */
  template <class K, class V, class IK = PageId>
  class Cache
  {
  public:
    /**
      An individual entry in the cache.
      */
    struct CacheEntry
    {
      K key;
      V value;

      CacheEntry(const CacheEntry& entry)
      : key(entry.key),
        value(entry.value)
      {
        // no code
      }

      CacheEntry(const K& key)
      : key(key)
      {
        // no code
      }

      CacheEntry(const K& key,
                 const V& value)
      : key(key),
        value(value)
      {
        // no code
      }
    };

    /**
      ValueSizer returns the size (in bytes) of an individual cache value.
      An implementation of ValueSizer has to be passed in the constructor of the cache
      to implement the GetSize() method.
      */
    class ValueSizer
    {
    public:
      virtual ~ValueSizer() = default;

      virtual size_t GetSize(const V& value) const = 0;
    };

    typedef std::list<CacheEntry>                              OrderList;
    typedef typename OrderList::iterator                       CacheRef;
    typedef std::unordered_map<K,typename OrderList::iterator> Map;

  private:
    size_t    size;
    size_t    maxSize;
    OrderList order;
    Map       map;
    CacheRef  previousEntry;

  private:

    inline IK KeyToInternalKey(K key)
    {
      return key - std::numeric_limits<K>::min();
    }

    /**
      Clear the cache deleting the oldest cache entries
      until it has the given max size.
      */
    void StripCache()
    {
      while (size>maxSize) {
        // Remove oldest entry from cache...

        // Get oldest entry an dremove it from the map
        map.erase(map.find(order.back().key));

        // Remove it from order list
        order.pop_back();

        previousEntry=order.end();

        size--;
      }
    }

  public:
    /**
     Create a new cache object with the given max size.
      */
    Cache(size_t maxSize)
     : size(0),
       maxSize(maxSize)
    {
      map.reserve(maxSize);
      previousEntry=order.end();
    }

    /**
     * Returns if the cache is active (maxSize > 0)
     */
    bool IsActive() const
    {
      return maxSize>0;
    }

    /**
      Getting the value with the given key from cache.

      If there is no valued stored with the given key, false will be
      returned and the reference will be untouched.

      If there is a value with the given key, reference will return
      a reference to the value and the value will moved to the
      front position of the cache to assure FIFO behaviour.
      */
    bool GetEntry(const K& key,
                  CacheRef& reference)
    {
      if (!IsActive()) {
        return false;
      }

      if (previousEntry!=order.end() &&
          previousEntry->key==key) {
        reference=previousEntry;
        return true;
      }

      typename Map::iterator iter=map.find(key);

      if (iter!=map.end()) {
        // Move key/value to the start of the order list
        order.splice(order.begin(),order,iter->second);

        // Update the map with the new iterator into the order list
        iter->second=order.begin();

        reference=order.begin();
        previousEntry=reference;

        return true;
      }

      return false;
    }

    /**
      Set or update the cache with the given value for the given key.

      If the key is not available in the cache the value will be added
      to the front of the cache (FIFO semantic) else the value will be updated
      (also moving it to the front of the cache (FIFO again).
      */
    typename Cache::CacheRef SetEntry(const CacheEntry& entry)
    {
      if (!IsActive()) {
        order.clear();

        order.push_front(entry);

        return order.begin();
      }

      typename Map::iterator iter=map.find(entry.key);

      if (iter!=map.end()) {
        // Move key/value to the start of the order list
        order.splice(order.begin(),order,iter->second);

        // Update the map with the new iterator into the order list
        iter->second=order.begin();

        order.front().value=entry.value;
      }
      else {
        // Place key/value to the start of the order list
        order.push_front(entry);
        size++;
        // Update the map with the new iterator into the order list
        map[entry.key]=order.begin();

        StripCache();
      }

      return order.begin();
    }

    /**
      Set a new cache max size, possible striping the oldest entries
      from cache if the new size is smaller than the old one.
      */
    void SetMaxSize(size_t maxSize)
    {
      this->maxSize=maxSize;

      StripCache();

      map.reserve(maxSize);
    }

    /**
     * Returns the maximum size of the cache
     */
    size_t GetMaxSize() const
    {
      return maxSize;
    }

    /**
      Completely flush the cache removing all entries from it.
      */
    void Flush()
    {
      order.clear();
      map.clear();
      size=0;
    }

    /**
      Returns the current size of the cache.
      */
    size_t GetSize() const
    {
      return size;
    }

    size_t GetMemory(const ValueSizer& sizer) const
    {
      size_t memory=0;

      // Size of map
      memory+=map.size()*sizeof(CacheRef);

      // Size of list
      memory+=size*sizeof(CacheEntry);

      for (typename std::list<CacheEntry>::const_iterator entry=order.begin();
           entry!=order.end();
           ++entry) {
        memory+=sizer.GetSize(entry->value);
      }

      return memory;
    }

    /**
      Dump some cache statistics to std::cout.
      */
    void DumpStatistics(const char* cacheName, const ValueSizer& sizer)
    {
      log.Debug() << cacheName << " entries: " << size << ", memory " << GetMemory(sizer);
    }
  };
}

#endif
