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

#include <cassert>
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
    LazyRef does delay the allocation fo the referenced object of type T. It also
    implements references counting on copy and assignment, allow to exchange
    costly copy operations with cheaper reference assignment operations. Using LazyRef
    is useful if you reference objects at multiple locations where the object is only
    destroyed if all locations delete the object and where copying the object is expensive.

    LazyRef allocates a new object instance the first time the reference is dereferenced and
    the object instance of type T is accessed.

    Note that type T must inherit from class Referencable!
  */
  template <typename T>
  class OSMSCOUT_API LazyRef
  {
  private:
    mutable T* ptr;

  public:
    /**
      Default constructor. Creates an empty reference.
    */
    inline LazyRef()
      : ptr(NULL)
    {
      // no code
    }

    /**
      Creates an reference holding an instance of T.
    */
    LazyRef(T* pointer)
      : ptr(pointer)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Creates an reference holding an instance of T.
    */
    LazyRef(const T& reference)
      : ptr(new T(reference))
    {
      ptr->AddReference();
    }

    /**
      Copy constructor for the same type of reference.
    */
    inline LazyRef(const LazyRef<T>& other)
      : ptr(other.ptr)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Destructor
    */
    inline ~LazyRef()
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
    void operator=(const LazyRef<T>& other)
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
  };

  /**
    Ref handles references to object using reference counting semantic. The object of type
    T is onbly deleted if all references have gone invalid.

    Note that type T must inherit from class Referencable!
  */
  template <typename T> class Ref
  {
  public:
    /**
      Default constructor. Creates an empty reference.
    */
    inline Ref()
      : ptr(NULL)
    {
      // no code
    }

    /**
      Creates an reference holding an instance of T.
    */
    inline Ref(T* pointer)
      : ptr(pointer)
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Copies object and creates new dynamic instance as copy.
      Expects type T to have a copy constructor!
    */
    /*
    inline Ref(const T& value)
    {
      ptr=new T(value);
      ptr->AddReference();
    }*/

    /**
      Copy constructor
    */
    inline Ref(const Ref<T>& other)
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
    inline Ref(const Ref<T1>& other)
      : ptr(other.Get())
    {
      if (ptr!=NULL) {
        ptr->AddReference();
      }
    }

    /**
      Destructor
    */
    inline ~Ref()
    {
      if (ptr!=NULL &&
          ptr->RemoveReference()==0) {
        delete ptr;
      }
    }

    /**
      Assignment operator.

      Assigns a new value to the reference. The reference count of the
      new object - if the pointer is not NULL - will be incremented.
      The reference count of the old value will be decremented and freed,
      if the count reached 0.
    */
    Ref<T>& operator=(T* pointer)
    {
      if (ptr!=pointer) {

        if (pointer!=NULL) {
          pointer->AddReference();
        }

        if (ptr!=NULL &&
            ptr->RemoveReference()==0) {
          delete ptr;
        }

        ptr=pointer;
      }

      return *this;
    }

    /**
      Assignment operator.

      Assigns a new value to the reference. The reference count of the
      object hold by the handed reference - if the pointer is not NULL -
      will be incremented. The reference count of the old value will be
      decremented and freed, if the count reached 0.
    */
    Ref<T>& operator=(const Ref<T>& other)
    {
      if (&other!=this && this->ptr!=other.ptr) {
        if (ptr!=NULL &&
            ptr->RemoveReference()==0) {
          delete ptr;
        }

        ptr=other.ptr;

        if (ptr!=NULL) {
          ptr->AddReference();
        }
      }

      return *this;
    }

    template<typename T1>
    Ref<T>& operator=(const Ref<T1>& other)
    {
      if (&other!=this && this->ptr!=other.Get()) {
        if (ptr!=NULL &&
            ptr->RemoveReference()) {
          delete ptr;
        }

        ptr=other.Get();

        if (ptr!=NULL) {
          ptr->AddReference();
        }
      }

      return *this;
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

      Returns the underlying object as reference. Allows reference to be
      passed as a parameter where the object type is required.
    */
    operator T&() const
    {
      return *ptr;
    }

    bool operator==(const Ref<T>& other) const
    {
      return ptr==other.ptr;
    }

    bool operator!=(const Ref<T>& other) const
    {
      return ptr!=other.ptr;
    }

    bool operator<(const Ref<T>& other) const
    {
      return ptr<other.ptr;
    }

  private:
    T* ptr;
  };
}

#endif
