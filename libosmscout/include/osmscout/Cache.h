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

#include <iostream>
#include <list>
#include <map>

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
  Cache(size_t maxSize)
   : maxSize(maxSize)
  {
    // no code
  }

  bool GetEntry(const K& key, CacheRef& reference) const
  {
    typename std::map<K,CacheRef>::const_iterator iter=map.find(key);

    if (iter==map.end()) {
      return false;
    }

    reference=iter->second;

    return true;
  }

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

  void SetMaxSize(size_t maxSize)
  {
    this->maxSize=maxSize;

    StripCache();
  }

  void Flush()
  {
    order.clear();
    map.clear();
  }

  size_t GetSize() const
  {
    return order.size();
  }

  void DumpStatistics(const char* cacheName, const ValueSizer& sizer)
  {
    size_t memory=0;

    memory+=order.size()*sizeof(CacheEntry);
    memory+=map.size()*sizeof(K)+map.size()*sizeof(CacheRef);

    for (typename std::list<CacheEntry>::const_iterator entry=order.begin();
         entry!=order.end();
         ++entry) {
      memory+=sizer.GetSize(entry->value);
    }

    std::cout << cacheName << " entries: " << order.size() << ", memory " << memory << std::endl;
  }
};

#endif
