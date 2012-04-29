#ifndef __JNI_OBJECT_ARRAY_H__
#define __JNI_OBJECT_ARRAY_H__

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <vector>

namespace osmscout
{
  template <class T>
  class JniObjectArray
  {
  private:

    std::vector<T*> objects;

  public:

    JniObjectArray()
    { 
    }

    ~JniObjectArray()
    {    
    }

    int Add(T* object)
    {
      for(int i=0; i<objects.size(); i++)
      {
        if (objects[i]==NULL)
        {
          // Found an empty slot
          objects[i]=object;

          return i;
        }
      }

      // No empty slot found. Add object at the end of the vector
      objects.push_back(object);

      return (objects.size()-1);
    }

    T* Get(int objectIndex)
    {
      if ((objectIndex<0) || (objectIndex>=objects.size()))
      {
        // Index is out of vector bounds
        return NULL;
      }

      return objects[objectIndex];
    }

    T* GetAndRemove(int objectIndex)
    {
      T* object=Get(objectIndex);

      objects[objectIndex]=NULL;

      return object;
    }
  };
}

#endif	// __JNI_OBJECT_ARRAY_H__

