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

#include <cassert>
#include <iostream>
#include <list>
#include <vector>

namespace osmscout {

  /**
    Generic FIFO cache implementation with O(n log n) semantic.

    Template parameter class K holds the key value (must be an unsigned numerical value),
    parameter class V holds the data class that is to be cached.

    * The cache is not threadsafe.
    * It uses a std::map for data lookup
    * It uses an std::list for implementing FIFO characteristics.
   */
  template <class K, class V>
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
    struct ValueSizer
    {
      virtual ~ValueSizer()
      {

      }

      virtual unsigned long GetSize(const V& value) const = 0;
    };

    typedef std::list<CacheEntry>                   OrderList;
    typedef typename OrderList::iterator            CacheRef;
    typedef std::list<typename OrderList::iterator> CacheRefList;
    typedef std::vector<CacheRefList>               Map;

  private:
    unsigned long size;
    unsigned long maxSize;
    OrderList     order;
    Map           map;

  private:
    /**
      Clear the cache deleting the oldest cache entries
      until it has the given max size.
      */
    void StripCache()
    {
      while (size>maxSize) {
        // Remove oldest entry from cache...

        // Get oldest entry
        typename std::list<CacheEntry>::reverse_iterator lastEntry=order.rbegin();

        unsigned long index=lastEntry->key%map.size();

        typename CacheRefList::iterator iter=map[index].begin();
        while (iter!=map[index].end() &&
               (*iter)->key!=lastEntry->key) {
          ++iter;
        }

        assert(iter!=map[index].end());

        // Remove it from map
        map[index].erase(iter);

        // Remove it from order list
        order.pop_back();
        size--;
      }
    }

  public:
    /**
     Create a new cache object with the given max size.
      */
    Cache(unsigned long maxSize)
     : size(0),
       maxSize(maxSize)
    {
      /*
      if (maxSize==0) {
        maxSize=1;
      }*/

      if(maxSize>=10) {
        map.resize(maxSize/5);
      }
      else {
        map.resize(maxSize);
      }
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
      if (map.size()==0) {
        return false;
      }

      unsigned long index(key%map.size());
      CacheRefList  *refList=&map[index];

      for (typename CacheRefList::iterator iter(refList->begin());
           iter!=refList->end();
           ++iter) {
        if ((*iter)->key==key) {
          // Move key/value to the start of the order list
          order.splice(order.begin(),order,*iter);

          // Update the map with the new iterator into the order list
          refList->push_front(order.begin());
          refList->erase(iter);

          reference=order.begin();

          return true;
        }
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
      if (map.size()==0) {
        order.clear();

        order.push_front(entry);

        return order.begin();
      }

      unsigned long                   index=entry.key%map.size();
      CacheRefList                    *refList=&map[index];
      typename CacheRefList::iterator iter=refList->begin();

      while (iter!=refList->end() &&
             (*iter)->key!=entry.key) {
        ++iter;
      }

      if (iter!=refList->end()) {
        // Move key/value to the start of the order list
        order.splice(order.begin(),order,*iter);

        // Update the map with the new iterator into the order list
        refList->push_front(order.begin());
        // Delete the old entry in the ref list
        refList->erase(iter);

        order.front().value=entry.value;
      }
      else {
        // Place key/value to the start of the order list
        order.push_front(entry);
        size++;
        // Update the map with the new iterator into the order list
        refList->push_front(order.begin());

        StripCache();
      }

      return order.begin();
    }

    /**
      Set a new cache max size, possible striping the oldest entries
      from cache if the new size is smaller than the old one.
      */
    void SetMaxSize(unsigned long maxSize)
    {
      /*
      if (maxSize==0) {
        maxSize=1;
      }*/

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
      size=0;

      if(maxSize>=10) {
        map.resize(maxSize/5);
      }
      else {
        map.resize(maxSize);
      }
    }

    /**
      Returns the current size of the cache.
      */
    unsigned long GetSize() const
    {
      return size;
    }

    unsigned long GetMemory(const ValueSizer& sizer) const
    {
      unsigned long memory=0;

      memory+=map.size()*sizeof(CacheRefList);
      memory+=size*sizeof(CacheEntry);

      for (unsigned long i=0; i<map.size(); i++) {
        memory+=map[i].size()*sizeof(CacheRef);
      }

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
      std::cout << cacheName << " entries: " << size << ", memory " << GetMemory(sizer) << std::endl;
    }
  };
}

#endif
