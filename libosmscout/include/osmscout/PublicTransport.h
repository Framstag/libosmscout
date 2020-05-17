#ifndef OSMSCOUT_PUBLIC_TRANSPORT_H
#define OSMSCOUT_PUBLIC_TRANSPORT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Tim Teulings

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

#include <memory>
#include <string>

#include <osmscout/CoreImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
   * \ingroup PublicTransport
   *
   * Representation of a public transport route
   */
  class OSMSCOUT_API PTRoute CLASS_FINAL
  {
  private:
    FileOffset  fileOffset;
    std::string name;
    std::string ref;
    std::string operatorName;
    std::string network;

  public:
    inline PTRoute() = default;

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetRef() const
    {
      return ref;
    }

    inline std::string GetOperator() const
    {
      return operatorName;
    }

    inline std::string GetNetwork() const
    {
      return network;
    }

    void SetName(const std::string& name);
    void SetRef(const std::string& ref);
    void SetOperator(const std::string& operatorName);
    void SetNetwork(const std::string& network);

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);

    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  using PTRouteRef = std::shared_ptr<PTRoute>;
}

#endif
