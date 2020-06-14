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

#include <osmscout/util/Color.h>
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
  public:
    /**
     * A halt can be of one of different types.
     */
    enum class StopType : uint8_t
    {
      normal    = 0,
      entryOnly = 1,
      exitOnly  = 2
    };

    /**
     * The platform of a halt can be of one of different types.
     */
    enum class PlatformType : uint8_t
    {
      normal    = 0,
      entryOnly = 1,
      exitOnly  = 2
    };

    /**
     * A route variant consists of a number of halts.
     */
    class OSMSCOUT_API Stop CLASS_FINAL
    {
    private:
      StopType      type;
      ObjectFileRef stop;

    public:
      inline StopType GetType() const
      {
        return type;
      }

      inline const ObjectFileRef& GetStop() const
      {
        return stop;
      }

      void SetType(StopType stopType);
      void SetStop(const ObjectFileRef& stop);

      friend PTRoute;
    };

    /**
     * A route variant consists of a number of platforms.
     */
    class OSMSCOUT_API Platform CLASS_FINAL
    {
    private:
      PlatformType  type;
      ObjectFileRef platform;

    public:
      inline PlatformType GetType() const
      {
        return type;
      }

      inline const ObjectFileRef& GetPlatform() const
      {
        return platform;
      }

      void SetType(PlatformType platformType);
      void SetPlatform(const ObjectFileRef& platform);

      friend PTRoute;
    };


    /**
     * A route can have multiple variants
     *
     * Normally you have one variant in one direction and a second variant in the
     * opposite direction.
     */
    class OSMSCOUT_API Variant CLASS_FINAL
    {
    private:
      std::string name;
      std::string ref;
      std::string operatorName;
      std::string network;
      std::string from;
      std::string to;
      Color       color;

    public:
      std::vector<Stop>     stops;
      std::vector<Platform> platforms;

    public:
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

      inline std::string GetFrom() const
      {
        return from;
      }

      inline std::string GetTo() const
      {
        return to;
      }

      inline Color GetColor() const
      {
        return color;
      }

      void SetName(const std::string& name);
      void SetRef(const std::string& ref);
      void SetOperator(const std::string& operatorName);
      void SetNetwork(const std::string& network);
      void SetFrom(const std::string& from);
      void SetTo(const std::string& to);
      void SetColor(const Color& color);

      friend PTRoute;
    };

  private:
    TypeInfoRef type;
    FileOffset  fileOffset;
    FileOffset  nextFileOffset;
    std::string name;
    std::string ref;
    std::string operatorName;
    std::string network;
    Color       color;

  public:
    std::vector<Variant> variants;

  public:
    inline PTRoute() = default;

    inline TypeInfoRef GetType() const {
      return type;
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline FileOffset GetNextFileOffset() const
    {
      return nextFileOffset;
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

    inline Color GetColor() const
    {
      return color;
    }

    void SetType(const TypeInfoRef& type);
    void SetName(const std::string& name);
    void SetRef(const std::string& ref);
    void SetOperator(const std::string& operatorName);
    void SetNetwork(const std::string& network);
    void SetColor(const Color& color);

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);

    void Write(const TypeConfig& typeConfig,
               FileWriter& writer) const;
  };

  using PTRouteRef = std::shared_ptr<PTRoute>;
}

#endif
