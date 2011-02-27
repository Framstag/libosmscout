#ifndef OSMSCOUT_UTIL_REFERENCE_H
#define OSMSCOUT_UTIL_REFERENCE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <cstdio>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  /**
    Baseclass for all classes that support reference counting.
  */
  class OSMSCOUT_API Referencable
  {
  public:
    Referencable()
      : count(0)
    {
      // no code
    }

    /**
      Add a reference to this object.

      Increments the internal reference counter.
    */
    inline void AddReference()
    {
      ++count;
    }

    /**
      Remove a reference from this object.

      Decrements the internal reference counter.
    */
    inline unsigned long RemoveReference()
    {
      count--;

      return count;
    }

    /**
      Returns the current reference count.
    */
    inline unsigned long GetReferenceCount() const
    {
      return count;
    }

  private:
    unsigned long count;
  };

  /**
    Autopointer like container class fo classes derived from Referencable
  */
  template <typename T>
  class OSMSCOUT_API Reference
  {
  private:
    mutable T* ptr;

  public:
    /**
      Default constructor. Creates an empty reference.
    */
    inline Reference()
      : ptr(NULL)
    {
      // no code
    }

    /**
      Creates an reference holding an instance of T.
    */
    Reference(T* pointer)
      : ptr(pointer)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Copy constructor for the same type of reference.
    */
    inline Reference(const Reference<T>& other)
      : ptr(other.ptr)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Destructor
    */
    inline ~Reference()
    {
      if (ptr!=NULL &&
          ptr->RemoveReference()==0) {
        delete ptr;
      }
    }

    /**
      Assignment operator.

      Assigns a new value to the reference. The reference count of the
      object hold by the handed reference - if the pointer is not NULL -
      will be incremented. The reference count of the old value will be
      decremented and freed, if the count reached 0.
    */
    void operator=(const Reference<T>& other)
    {
      if (ptr!=other.ptr) {
        if (ptr!=NULL &&
            ptr->RemoveReference()==0) {
          delete ptr;
        }

        ptr=other.ptr;

        if (ptr!=NULL) {
          ptr->AddReference();
        }
      }
    }

    /**
      arrow operator.

      Returns the underlying pointer. Makes the reference behave like a pointer.
    */
    T* operator->() const
    {
      if (ptr==NULL) {
        ptr=new T();
        ptr->AddReference();
      }

      return ptr;
    }
  };
}

#endif
