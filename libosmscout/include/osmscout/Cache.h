#ifndef OSMSCOUT_CACHE_H
#define OSMSCOUT_CACHE_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
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

#include <cassert>
#include <iostream>
#include <list>
#include <map>

namespace osmscout {

  /**
    Generic FIFO cache implementation with O(n log n) semantic.
   */
  template <class K, class V>
  class Cache
  {
  public:
    struct CacheEntry
    {
      K key;
      V value;

      CacheEntry(const K& key)
      : key(key)
      {
        // no code
      }

      CacheEntry(const K& key, const V& value)
      : key(key),
        value(value)
      {
        // no code
      }
    };

    struct ValueSizer
    {
      virtual size_t GetSize(const V& value) const = 0;
    };

    typedef typename std::list<CacheEntry>::iterator CacheRef;

  private:
    size_t                maxSize;
    std::list<CacheEntry> order;
    std::map<K,CacheRef>  map;

  private:
    /**
      Clear the cache deleting the oldest cache entries
      until it has the given max size.
      */
    void StripCache()
    {
      while (order.size()>maxSize) {
        // Remove oldest entry from cache...

        // Get oldest entry
        typename std::list<CacheEntry>::reverse_iterator lastEntry=order.rbegin();

        // Remove it from map
        map.erase(lastEntry->key);

        // Remove it from order list
        order.pop_back();
      }
    }

  public:
    /**
     Create a new cache object with the given max size.
      */
    Cache(size_t maxSize)
     : maxSize(maxSize)
    {
      assert(maxSize>0);
    }

    /**
      Getting the value witht he given key from cache.

      If there is no valued stored with the given key, false will be
      returned and the reference will be untouched.

      If there is a value with the given key, reference will return
      a reference to the value and the value will move to the
      from position of the cache to assure FIFO behaviour.
      */
    bool GetEntry(const K& key, CacheRef& reference)
    {
      typename std::map<K,CacheRef>::iterator iter=map.find(key);

      if (iter==map.end()) {
        return false;
      }

      CacheRef next=iter->second;

      next++;

      // Place key/value to the start of the order list
      order.insert(order.begin(),iter->second,next);

      // Erase the entry from its current order list position
      order.erase(iter->second);


      // Update the map with the new iterator into the order list
      iter->second=order.begin();

      reference=iter->second;

      return true;
    }

    /**
      Set or update the cache witht he given value for the given key.

      If the key is not available in the cache the value will be added
      to the front of the cache (FIFO semantic) else the value will be updated
      (also moving it to the front of the cache (FIFO again).
      */
    typename Cache::CacheRef SetEntry(const CacheEntry& entry)
    {
      typename std::map<K,CacheRef>::iterator iter=map.find(entry.key);

      if (iter==map.end()) {
        //Insert

        // Place key/value to the start of the order list
        order.push_front(entry);

        // Insert entry into the map
        map[entry.key]=order.begin();
      }
      else {
        // Update

        // Erase the entry from its current order list position
        order.erase(iter->second);

        // Place key/value to the start of the order list
        order.push_front(entry);

        // Update the map with the new iterator into the order list
        iter->second=order.begin();
      }

      StripCache();

      return order.begin();
    }

    /**
      Set a new cache max size, possible striping the oldest entries
      from cache if the new size is smaller than the old one.
      */
    void SetMaxSize(size_t maxSize)
    {
      assert(maxSize>0);

      this->maxSize=maxSize;

      StripCache();
    }

    /**
      Completely flush the cache removing all entries from it.
      */
    void Flush()
    {
      order.clear();
      map.clear();
    }

    /**
      Returns the current size of the cache.
      */
    size_t GetSize() const
    {
      return order.size();
    }

    size_t GetMemory(const ValueSizer& sizer) const
    {
      size_t memory=0;

      memory+=order.size()*sizeof(CacheEntry);
      memory+=map.size()*sizeof(K)+map.size()*sizeof(CacheRef);

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
      std::cout << cacheName << " entries: " << order.size() << ", memory " << GetMemory(sizer) << std::endl;
    }
  };
}

#endif
