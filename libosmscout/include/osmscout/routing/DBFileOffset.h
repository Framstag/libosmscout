#ifndef OSMSCOUT_DBFILEOFFSET_H
#define OSMSCOUT_DBFILEOFFSET_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Lukas Karas

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

#include <string>
#include <ostream>
#include <osmscout/Types.h>

namespace osmscout{

  typedef uint32_t DatabaseId;

  /**
   * \ingroup Routing
   *
   * Helper structure to implement a reference to a routing node in a given
   * database (identified by a unique index).
   */
  struct DBId
  {
    DatabaseId database;
    Id         id;

    DBId()
      : database(0),
        id(0)
    {
    }

    DBId(const DBId &o)
      : database(o.database),
        id(o.id)
    {
    }

    DBId(DatabaseId database,
         Id id)
      : database(database),
        id(id)
    {
    }

    inline bool IsValid() const
    {
      return id!=0;
    }

    inline bool operator==(const DBId& other) const
    {
      return database==other.database && id==other.id;
    }

    inline bool operator!=(const DBId& other) const
    {
      return database!=other.database || id!=other.id;
    }

    inline bool operator<(const DBId& other) const
    {
      if (database!=other.database) {
        return database<other.database;
      }

      return id<other.id;
    }

    DBId& operator=(const DBId& other)
    {
      if(&other==this) {
        return *this;
      }

      this->database=other.database;
      this->id=other.id;

      return *this;
    }
  };

  inline std::ostream& operator<<(std::ostream &stream,const DBId &o)
  {
    stream << "DBId(" << o.database << "," << o.id << ")";
    return stream;
  }

  /**
   * \ingroup Routing
   *
   * Helper structure to implement a reference to a routing node in a given
   * database (identified by a unique index).
   */
  struct DBFileOffset
  {
    DatabaseId database;
    FileOffset offset;

    DBFileOffset()
    : database(0),
      offset(0)
    {
    }

    DBFileOffset(const DBFileOffset &o)
    : database(o.database),
      offset(o.offset)
    {
    }

    DBFileOffset(DatabaseId database,
                 FileOffset offset)
    : database(database),
      offset(offset)
    {
    }

    inline bool IsValid() const
    {
      return offset!=0;
    }

    inline bool operator==(const DBFileOffset& other) const
    {
      return database==other.database && offset==other.offset;
    }

    inline bool operator!=(const DBFileOffset& other) const
    {
      return database!=other.database || offset!=other.offset;
    }

    inline bool operator<(const DBFileOffset& other) const
    {
      if (database!=other.database) {
        return database<other.database;
      }

      return offset<other.offset;
    }

    DBFileOffset& operator=(const DBFileOffset& other)
    {
      if(&other==this) {
        return *this;
      }

      this->database=other.database;
      this->offset=other.offset;

      return *this;
    }
  };

  inline std::ostream& operator<<(std::ostream &stream,const DBFileOffset &o)
  {
    stream << "DBFileOffset(" << o.database << "," << o.offset << ")";
    return stream;
  }
}

namespace std {
  template <>
  struct hash<osmscout::DBId>
  {
    size_t operator()(const osmscout::DBId& id) const
    {
      return hash<uint32_t>{}(id.database) ^ hash<osmscout::Id>{}(id.id);
    }
  };

  template <>
  struct hash<osmscout::DBFileOffset>
  {
    size_t operator()(const osmscout::DBFileOffset& id) const
    {
      return hash<uint32_t>{}(id.database) ^ hash<osmscout::FileOffset>{}(id.offset);
    }
  };
}

#endif /* OSMSCOUT_DBFILEOFFSET_H */
