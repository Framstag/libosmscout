#ifndef OSMSCOUT_IMPORT_GENGENROUTEMEMBERINDEX_H
#define OSMSCOUT_IMPORT_GENGENROUTEMEMBERINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukáš Karas

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

#include <osmscout/import/GenNumericIndex.h>
#include <osmscout/import/RawRelation.h>

#include <osmscout/system/Compiler.h>

#include <iostream>

namespace osmscout {

  struct RouteMember
  {
    OSMId id; // id of way

    struct RouteMemberEntry
    {
      FileOffset route; // offset of RawRoute
    };

    OSMId GetId() const
    {
      return id;
    }
  };

  class GenRouteMemberIndex CLASS_FINAL : public NumericIndexGenerator<OSMId,RouteMember>
  {
  public:
    static const char* const ROUTEMEMBER_IDX;

  public:
    GenRouteMemberIndex();

    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;
  private:

    void ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  RouteMember& data) const
    {
      //data.Read(typeConfig,
      //          scanner);
      std::cout << "read route member" << std::endl;
    }

  private:
    std::list<RouteMember> stack;
  };
}

#endif
