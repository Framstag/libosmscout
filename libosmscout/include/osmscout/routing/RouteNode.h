#ifndef OSMSCOUT_ROUTENODE_H
#define OSMSCOUT_ROUTENODE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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
#include <vector>

#include <osmscout/ObjectRef.h>
#include <osmscout/Path.h>
#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * For every unique combination of object attributes that are routing
   * relevant we store an ObjectvariantData entry.
   */
  struct OSMSCOUT_API ObjectVariantData
  {
  public:
    TypeInfoRef type;     //!< The type of the object
    uint8_t     maxSpeed; //!< Maximum speed allowed on the way
    uint8_t     grade;    //!< Quality of road/track 1 (good)...5 (bad)

    bool operator==(const ObjectVariantData& other) const;
    bool operator<(const ObjectVariantData& other) const;

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(FileWriter& writer) const;
  };

  /**
   * \ingroup Routing
   * A route node is the representation of a node in the routing graph.
   */
  class OSMSCOUT_API RouteNode
  {
  public:
    static const uint8_t usableByFoot         = 1 << 0; //!< This path can be traveled by foot
    static const uint8_t usableByBicycle      = 1 << 1; //!< This path can be traveled by bicycle
    static const uint8_t usableByCar          = 1 << 2; //!< This path can be traveled by car
    static const uint8_t restrictedForFoot    = 1 << 3; //!< Using this path ist restricted for foot
    static const uint8_t restrictedForBicycle = 1 << 4; //!< Using this path ist restricted for bicycle
    static const uint8_t restrictedForCar     = 1 << 5; //!< Using this path ist restricted for car

    /**
     * \ingroup Routing
     * Information for an object referenced by a path.
     */
    struct OSMSCOUT_API ObjectData
    {
      ObjectFileRef object;             //!< Reference to the object
      uint16_t      objectVariantIndex; //!< Index into the lookup table, holding object specific routing data
    };

    /**
     * \ingroup Routing
     * Exclude regarding use of paths. You cannot use the path with the index "targetPath" if you come
     * from the source object.
     */
    struct OSMSCOUT_API Exclude
    {
      ObjectFileRef source;      //!< The source object
      uint8_t       targetIndex; //!< The index of the target path
    };

    /**
      * \ingroup Routing
     * A single path that starts at the given route node. A path contains a number of information
     * that are relevant for the router.
     */
    struct OSMSCOUT_API Path
    {
      double     distance;    //!< Distance from the current route node to the target route node
      Id         id;          //!< id of the targeting route node
      uint8_t    objectIndex; //!< The index of the way to use from this route node to the target route node
      uint8_t    flags;       //!< Certain flags
      //uint8_t    bearing;   //!< Encoded initial and final bearing of this path

      inline bool IsRestricted(Vehicle vehicle) const
      {
        switch (vehicle) {
        case vehicleFoot:
          return (flags & restrictedForFoot) != 0;
        case vehicleBicycle:
          return (flags & restrictedForBicycle) != 0;
        case vehicleCar:
          return (flags & restrictedForCar) != 0;
        }

        return false;
      }
    };

  private:
    FileOffset              fileOffset; //!< FileOffset of the route node
    Point                   point;      //!< Coordinate and id of the route node

  public:
    std::vector<ObjectData> objects;    //!< List of objects (ways, areas) that cross this route node
    std::vector<Path>       paths;      //!< List of paths that can in principle be used from this node
    std::vector<Exclude>    excludes;   //!< List of potential excludes regarding use of paths

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline Id GetId() const
    {
      return point.GetId();
    }

    inline GeoCoord GetCoord() const
    {
      return point.GetCoord();
    }

    inline void Initialize(FileOffset fileOffset,
                           const Point& point)
    {
      this->fileOffset=fileOffset;
      this->point=point;
    }

    uint8_t AddObject(const ObjectFileRef& object,
                      uint16_t objectVariantIndex);

    void Read(FileScanner& scanner);
    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(FileWriter& writer) const;
  };

  typedef std::shared_ptr<RouteNode> RouteNodeRef;
}

#endif
