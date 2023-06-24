#ifndef OSMSCOUT_OBJECTREF_H
#define OSMSCOUT_OBJECTREF_H

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

#include <osmscout/lib/CoreImportExport.h>

#include <string>
#include <tuple>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  enum OSMRefType
  {
    osmRefNone     = 0,
    osmRefNode     = 1,
    osmRefWay      = 2,
    osmRefRelation = 3
  };

  /**
   * Reference to an OSM object by its type (Node, Way, Relation) and its
   * OSM object id.
   */
  class OSMSCOUT_API ObjectOSMRef CLASS_FINAL
  {
  public:
    OSMId      id=0; //NOLINT
    OSMRefType type=osmRefNone; // NOLINT

  public:
    ObjectOSMRef() = default;
    ObjectOSMRef(const ObjectOSMRef& ref) = default;
    ObjectOSMRef(ObjectOSMRef&& ref) = default;

    ObjectOSMRef(OSMId id,
                 OSMRefType type)
    : id(id),
      type(type)
    {
      // no code
    }

    ~ObjectOSMRef() = default;

    ObjectOSMRef& operator=(const ObjectOSMRef& other) = default;
    ObjectOSMRef& operator=(ObjectOSMRef&& other) = default;

    bool operator<(const ObjectOSMRef& reference) const
    {
      return std::tie(type, id) < std::tie(reference.type, reference.id);
    }

    bool operator==(const ObjectOSMRef& reference) const
    {
      return type==reference.type && id==reference.id;
    }

    bool operator!=(const ObjectOSMRef& reference) const
    {
      return type!=reference.type || id!=reference.id;
    }

    void Set(const OSMId& id,
                    const OSMRefType& type)
    {
      this->id=id;
      this->type=type;
    }

    void Invalidate()
    {
      this->id=0;
      this->type=osmRefNone;
    }

    const OSMId& GetId() const
    {
      return id;
    }

    const OSMRefType& GetType() const
    {
      return type;
    }

    std::string GetName() const;

    bool Valid() const
    {
      return type!=osmRefNone;
    }

    bool Invalid() const
    {
      return type==osmRefNone;
    }

    bool IsNode() const
    {
      return type==osmRefNode;
    }

    bool IsWay() const
    {
      return type==osmRefWay;
    }

    bool IsRelation() const
    {
      return type==osmRefRelation;
    }

    const char* GetTypeName() const;
  };

  enum RefType
  {
    refNone     = 0,
    refNode     = 1,
    refArea     = 2,
    refWay      = 3
  };

  /**
   * Reference to an libosmscout internal object by its type (area, way, node)
   * and by its file offset within its data file.
   */
  class OSMSCOUT_API ObjectFileRef CLASS_FINAL
  {
  public:
    FileOffset offset=0; // NOLINT
    RefType    type=refNone; // NOLINT

  public:
    ObjectFileRef() = default;
    ObjectFileRef(const ObjectFileRef& ref) = default;
    ObjectFileRef(ObjectFileRef&& ref) = default;

    ObjectFileRef(FileOffset offset,
                  RefType type)
    : offset(offset),
      type(type)
    {
      // no code
    }

    ~ObjectFileRef() = default;

    ObjectFileRef& operator=(const ObjectFileRef& other) = default;
    ObjectFileRef& operator=(ObjectFileRef&& other) = default;

    bool operator<(const ObjectFileRef& reference) const
    {
      return std::tie(type, offset) < std::tie(reference.type, reference.offset);
    }

    bool operator==(const ObjectFileRef& reference) const
    {
      return type==reference.type && offset==reference.offset;
    }

    bool operator!=(const ObjectFileRef& reference) const
    {
      return type!=reference.type || offset!=reference.offset;
    }

    void Set(const FileOffset& offset,
             const RefType& type)
    {
      this->offset=offset;
      this->type=type;
    }

    void Invalidate()
    {
      this->offset=0;
      this->type=refNone;
    }

    const FileOffset& GetFileOffset() const
    {
      return offset;
    }

    const RefType& GetType() const
    {
      return type;
    }

    std::string GetName() const;

    bool Valid() const
    {
      return type!=refNone;
    }

    bool Invalid() const
    {
      return type==refNone;
    }

    bool IsNode() const
    {
      return type==refNode;
    }

    bool IsWay() const
    {
      return type==refWay;
    }

    bool IsArea() const
    {
      return type==refArea;
    }

    const char* GetTypeName() const;
  };


  /**
   * Comparator to sort ObjectFileRefs strictly by increasing file offset
   */
  class OSMSCOUT_API ObjectFileRefByFileOffsetComparator CLASS_FINAL
  {
    public:
    bool operator()(const ObjectFileRef& a,
                           const ObjectFileRef& b) const
    {
      return a.GetFileOffset()<b.GetFileOffset();
    }
  };
}

#endif
