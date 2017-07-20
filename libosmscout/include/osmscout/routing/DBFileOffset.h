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

  struct DBFileOffset{
    DatabaseId database;
    FileOffset offset;

    DBFileOffset():
      database(0),offset(0)
    {
    }

    DBFileOffset(const DBFileOffset &o):
      database(o.database),offset(o.offset)
    {
    }

    DBFileOffset(DatabaseId database,FileOffset offset):
     database(database),offset(offset)
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
      if (database!=other.database)
        return database<other.database;
      return offset<other.offset;
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
  struct hash<osmscout::DBFileOffset>
  {
    size_t operator()(const osmscout::DBFileOffset& offset) const
    {
      return hash<uint32_t>{}(offset.database) ^ hash<osmscout::FileOffset>{}(offset.offset);
    }
  };
}

#endif /* OSMSCOUT_DBFILEOFFSET_H */
