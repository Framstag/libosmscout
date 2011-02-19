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

#include <stdio.h>

#include <cassert>

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
      Add a reference to this object
    */
    void AddReference();

    /**
      Remove a reference from this object
    */
    void RemoveReference();

    /**
      Returns the current reference count.
    */
    unsigned long GetReferenceCount() const;

  private:
    unsigned long count;
  };

  /**
    Autopointer like container class fo classes derived from Referencable
  */
  template <typename T>
  class OSMSCOUT_API Reference
  {
  public:
    /**
      Default constructor. Creates an empty reference.
    */
    Reference()
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
      Copy constructor
    */
    Reference(const Reference<T>& other)
      : ptr(other.ptr)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Access operator.

      Returns the underlying pointer. Note that the object is still
      hold by the Reference.
    */
    T* Get() const
    {
      return ptr;
    }

    /**
      Copy constructor
    */
    template<typename T1>
    Reference(const Reference<T1>& other)
      : ptr(other.Get())
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Destructor
    */
    ~Reference()
    {
      if (ptr!=NULL) {
        ptr->RemoveReference();
      }
    }

    /**
      Assignment operator.

      Assigns a new value to the reference. The reference count of the
      new object - if the pointer is not NULL - will be incremented.
      The reference count of the old value will be decremented and freed,
      if the count reached 0.
    */
    void operator=(T* pointer)
    {
      if (ptr!=pointer) {

        if (pointer!=NULL) {
          pointer->AddReference();
        }

        if (ptr!=NULL) {
          ptr->RemoveReference();
        }

        ptr=pointer;
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
      if (&other!=this && this->ptr!=other.ptr) {
        if (ptr!=NULL) {
          ptr->RemoveReference();
        }

        ptr=other.ptr;

        if (ptr!=NULL) {
          ptr->AddReference();
        }
      }
    }

    template<typename T1>
    void operator=(const Reference<T1>& other)
    {
      if (&other!=this && this->ptr!=other.Get()) {
        if (ptr!=NULL) {
          ptr->RemoveReference();
        }

        ptr=other.Get();

        if (ptr!=NULL) {
          ptr->AddReference();
        }
      }
    }

    bool operator==(const T* other) const
    {
      return ptr==other;
    }

    bool operator==(const Reference<T>& other) const
    {
      return ptr==other.ptr;
    }

    bool operator!=(const Reference<T>& other) const
    {
      return ptr!=other.ptr;
    }

    bool operator<(const Reference<T>& other) const
    {
      return ptr<other.ptr;
    }

    bool Valid() const
    {
      return ptr!=NULL;
    }

    bool Invalid() const
    {
      return ptr==NULL;
    }

    /**
      arrow operator.

      Returns the underlying pointer. Makes the reference behave like a pointer.
    */
    T* operator->() const
    {
      assert(ptr!=NULL); // Method calling on NULL pointer is forbidden
      return ptr;
    }

    /**
      Dereference operator.

      Returns a reference to the underlying object. Makes the reference behave
      like a pointer.
    */
    T& operator*() const
    {
      assert(ptr!=NULL);

      return *ptr;
    }

    /**
      Type conversion operator.

      Returns the underlying pointer. Allows reference to be
      passed as a parameter where the base pointer type is required.
    */
    operator T*() const
    {
      return ptr;
    }

    /**
      Type conversion operator.

      Returns the underlying pointer. Allows reference to be
      passed as a parameter where the base pointer type is required.
    */
    operator T&() const
    {
      return ptr;
    }

  private:
    T* ptr;
  };
}

#endif
