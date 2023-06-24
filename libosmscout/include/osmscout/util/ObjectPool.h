#ifndef OSMSCOUT_OBJECTPOOL_H
#define OSMSCOUT_OBJECTPOOL_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukas Karas

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

#include <osmscout/lib/CoreImportExport.h>

#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace osmscout {

  template<typename T>
  class ObjectPool {
  private:
    std::vector<T*> pool;
    size_t maxSize;
    std::mutex mutex;

  private:
    void Return(T* o){
      std::scoped_lock<std::mutex> guard(mutex);
      if (!IsValid(o) || pool.size()==maxSize){
        Destroy(o);
      } else {
        pool.push_back(o);
      }
    }

  public:
    using Ptr = std::unique_ptr<T, std::function<void(T*)>>;

  public:
    explicit ObjectPool(size_t maxSize):
        maxSize(maxSize)
    {
      pool.reserve(std::min((size_t)100, maxSize));
    }

    virtual ~ObjectPool(){
      Clear();
    }

    /**
     * Make a new object. It may return nullptr in case of failure.
     */
    virtual T* MakeNew() noexcept = 0;

    virtual void Destroy(T* o) noexcept
    {
      delete o;
    }

    virtual bool IsValid(T*) noexcept
    {
      return true;
    }

    virtual Ptr Borrow()
    {
      std::scoped_lock<std::mutex> guard(mutex);
      T* o;
      if (!pool.empty()){
        o=pool.back();
        pool.pop_back();
      } else {
        o=MakeNew();
        if (o == nullptr){
          return std::unique_ptr<T>(nullptr);
        }
      }
      return Ptr(o, [this](T* o){ Return(o); });
    }

    size_t Size()
    {
      std::scoped_lock<std::mutex> guard(mutex);
      return pool.size();
    }

    void Clear()
    {
      std::scoped_lock<std::mutex> guard(mutex);
      for (T* o:pool){
        Destroy(o);
      }
      pool.clear();
    }
  };

}

#endif //OSMSCOUT_OBJECTPOOL_H
