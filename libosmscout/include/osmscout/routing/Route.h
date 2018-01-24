#ifndef OSMSCOUT_ROUTE_H
#define OSMSCOUT_ROUTE_H

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

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <osmscout/ObjectRef.h>
#include <osmscout/Path.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/routing/DBFileOffset.h>

namespace osmscout {

  /**
   * \ingroup Routing
   * Description of a route, enhanced with information that are required to
   * give a human textual (or narrative) drive instructions;
   *
   * A route consists of nodes. A Node can be the crossing point of a number of
   * ways and is a route decision point (where the driver possibly has the change ways)
   * that requires some potential action by the driver.
   *
   * For each node you can pass a number of descriptions. For the way from the current node
   * to the next node also a number of descriptions can get retrieved.
   *
   * Descriptions are typed and must derive from class Description..
   */
  class OSMSCOUT_API RouteDescription
  {
  public:
    /** Constant for a description of the start node (StartDescription) */
    static const char* const NODE_START_DESC;
    /** Constant for a description of the target node (TargetDescription) */
    static const char* const NODE_TARGET_DESC;
    /** Constant for a description of name of the way (NameDescription) */
    static const char* const WAY_NAME_DESC;
    /** Constant for a description of a change of way name (NameChangedDescription) */
    static const char* const WAY_NAME_CHANGED_DESC;
    /** Constant for a description of list of way name crossing a node (CrossingWaysDescription) */
    static const char* const CROSSING_WAYS_DESC;
    /** Constant for a description of drive direction (DirectionDescription) */
    static const char* const DIRECTION_DESC;
    /** Constant for a description of an explicit turn (TurnDescription) */
    static const char* const TURN_DESC;
    /** Constant for a description of entering a roundabout  (RoundaboutEnterDescription) */
    static const char* const ROUNDABOUT_ENTER_DESC;
    /** Constant for a description of leaving a roundabout (RoundaboutLeaveDescription) */
    static const char* const ROUNDABOUT_LEAVE_DESC;
    /** Constant for a description of entering a motorway (MotorwayEnterDescription) */
    static const char* const MOTORWAY_ENTER_DESC;
    /** Constant for a description of changing a motorway (MotorwayChangeDescription) */
    static const char* const MOTORWAY_CHANGE_DESC;
    /** Constant for a description of leaving a motorway (MotorwayLeaveDescription) */
    static const char* const MOTORWAY_LEAVE_DESC;
    /** Constant for a description of node describing a motorway junction */
    static const char* const MOTORWAY_JUNCTION_DESC;
    /** Constant for a description of a destination to choose at a junction */
    static const char* const CROSSING_DESTINATION_DESC;
    /** Constant for a description of the maximum speed for the given way */
    static const char* const WAY_MAXSPEED_DESC;
    /** Constant for a description of type name of the way (TypeNameDescription) */
    static const char* const WAY_TYPE_NAME_DESC;

  public:
    /**
     * \ingroup Routing
     * Base class of all descriptions.
     */
    class OSMSCOUT_API Description
    {
    public:
      virtual ~Description();

      virtual std::string GetDebugString() const = 0;
    };

    typedef std::shared_ptr<Description> DescriptionRef;

    /**
     * \ingroup Routing
     * Start of the route
     */
    class OSMSCOUT_API StartDescription : public Description
    {
    private:
      std::string description;

    public:
      StartDescription(const std::string& description);

      std::string GetDebugString() const;

      std::string GetDescription() const;
    };

    typedef std::shared_ptr<StartDescription> StartDescriptionRef;

    /**
     * \ingroup Routing
     * Target of the route
     */
    class OSMSCOUT_API TargetDescription : public Description
    {
    private:
      std::string description;

    public:
      TargetDescription(const std::string& description);

      std::string GetDebugString() const;

      std::string GetDescription() const;
    };

    typedef std::shared_ptr<TargetDescription> TargetDescriptionRef;

    /**
     * \ingroup Routing
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (LIke B1 or A40).
     */
    class OSMSCOUT_API NameDescription : public Description
    {
    private:
      std::string name;
      std::string ref;

    public:
      NameDescription(const std::string& name);

      NameDescription(const std::string& name,
                      const std::string& ref);

      std::string GetDebugString() const;

      bool HasName() const;

      std::string GetName() const;
      std::string GetRef() const;

      std::string GetDescription() const;
    };

    typedef std::shared_ptr<NameDescription> NameDescriptionRef;

    /**
     * \ingroup Routing
     * Something has a name. A name consists of a name and a optional alphanumeric
     * reference (Like B1 or A40).
     */
    class OSMSCOUT_API NameChangedDescription : public Description
    {
      NameDescriptionRef originDescription;
      NameDescriptionRef targetDescription;

    public:
      NameChangedDescription(const NameDescriptionRef& originDescription,
                             const NameDescriptionRef& targetDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetOriginDesccription() const
      {
        return originDescription;
      }

      inline const NameDescriptionRef& GetTargetDesccription() const
      {
        return targetDescription;
      }
    };

    typedef std::shared_ptr<NameChangedDescription> NameChangedDescriptionRef;

    /**
     * \ingroup Routing
     * List the names of all ways, that are crossing the current node.
     */
    class OSMSCOUT_API CrossingWaysDescription : public Description
    {
    private:
      size_t                        exitCount;
      NameDescriptionRef            originDescription;
      NameDescriptionRef            targetDescription;
      std::list<NameDescriptionRef> descriptions;

    public:
      CrossingWaysDescription(size_t exitCount,
                              const NameDescriptionRef& originDescription,
                              const NameDescriptionRef& targetDescription);

      void AddDescription(const NameDescriptionRef& description);

      std::string GetDebugString() const;

      inline size_t GetExitCount() const
      {
        return exitCount;
      }

      inline bool HasMultipleExits() const
      {
        return exitCount>1;
      }

      inline const NameDescriptionRef& GetOriginDesccription() const
      {
        return originDescription;
      }

      inline const NameDescriptionRef& GetTargetDesccription() const
      {
        return targetDescription;
      }

      inline const std::list<NameDescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }
    };

    typedef std::shared_ptr<CrossingWaysDescription> CrossingWaysDescriptionRef;

    /**
     * \ingroup Routing
     * Describes the turn and the curve while getting from the previous node to the next node via the current node.
     *
     * The turn is the angle between the incoming way (previous node and current node)
     * and the outgoing way (current node and next node) at the given node.
     *
     * The curve is a heuristic measurement that not only take the next node of the target way into
     * account (which could only the start of a slight curve) but tries to determine the last node
     * of the curve and this gives a better description of the curve the vehicle needs to take.
     */
    class OSMSCOUT_API DirectionDescription : public Description
    {
    public:
      enum Move {
        sharpLeft,
        left,
        slightlyLeft,
        straightOn,
        slightlyRight,
        right,
        sharpRight
      };

    private:
      double turnAngle;
      double curveAngle;
      Move   turn;
      Move   curve;

    private:
      Move ConvertAngleToMove(double angle) const;
      std::string ConvertMoveToString(Move move) const;

    public:
      DirectionDescription(double turnAngle,
                      double curveAngle);

      std::string GetDebugString() const;

      inline double GetTurnAngle() const
      {
        return turnAngle;
      }

      inline double GetCurveAngle() const
      {
        return curveAngle;
      }

      inline Move GetTurn() const
      {
        return turn;
      }

      inline Move GetCurve() const
      {
        return curve;
      }
    };

    typedef std::shared_ptr<DirectionDescription> DirectionDescriptionRef;

    /**
     * \ingroup Routing
     * Signals an explicit turn
     */
    class OSMSCOUT_API TurnDescription : public Description
    {
    public:
      TurnDescription();

      std::string GetDebugString() const;
    };

    typedef std::shared_ptr<TurnDescription> TurnDescriptionRef;

    /**
     * \ingroup Routing
     * Signals entering a roundabout
     */
    class OSMSCOUT_API RoundaboutEnterDescription : public Description
    {
    public:
      RoundaboutEnterDescription();

      std::string GetDebugString() const;
    };

    typedef std::shared_ptr<RoundaboutEnterDescription> RoundaboutEnterDescriptionRef;

    /**
     * \ingroup Routing
     * Signals leaving a roundabout
     */
    class OSMSCOUT_API RoundaboutLeaveDescription : public Description
    {
    private:
      size_t exitCount;

    public:
      RoundaboutLeaveDescription(size_t exitCount);

      std::string GetDebugString() const;

      inline size_t GetExitCount() const
      {
        return exitCount;
      }
    };

    typedef std::shared_ptr<RoundaboutLeaveDescription> RoundaboutLeaveDescriptionRef;

    /**
     * \ingroup Routing
     * Signals entering a motorway
     */
    class OSMSCOUT_API MotorwayEnterDescription : public Description
    {
    private:
      NameDescriptionRef toDescription;

    public:
      MotorwayEnterDescription(const NameDescriptionRef& toDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetToDescription() const
      {
        return toDescription;
      }
    };

    typedef std::shared_ptr<MotorwayEnterDescription> MotorwayEnterDescriptionRef;

    /**
     * \ingroup Routing
     * Signals changing a motorway
     */
    class OSMSCOUT_API MotorwayChangeDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;
      NameDescriptionRef toDescription;

    public:
      MotorwayChangeDescription(const NameDescriptionRef& fromDescription,
                                const NameDescriptionRef& toDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetFromDescription() const
      {
        return fromDescription;
      }

      inline const NameDescriptionRef& GetToDescription() const
      {
        return toDescription;
      }
    };

    typedef std::shared_ptr<MotorwayChangeDescription> MotorwayChangeDescriptionRef;

    /**
     * \ingroup Routing
     * Signals leaving a motorway
     */
    class OSMSCOUT_API MotorwayLeaveDescription : public Description
    {
    private:
      NameDescriptionRef fromDescription;

    public:
      MotorwayLeaveDescription(const NameDescriptionRef& fromDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetFromDescription() const
      {
        return fromDescription;
      }
    };

    typedef std::shared_ptr<MotorwayLeaveDescription> MotorwayLeaveDescriptionRef;

    /**
     * \ingroup Routing
     * A motorway junction
     */
    class OSMSCOUT_API MotorwayJunctionDescription : public Description
    {
    private:
      NameDescriptionRef junctionDescription;

    public:
      MotorwayJunctionDescription(const NameDescriptionRef& junctionDescription);

      std::string GetDebugString() const;

      inline const NameDescriptionRef& GetJunctionDescription() const
      {
        return junctionDescription;
      }
    };

    typedef std::shared_ptr<MotorwayJunctionDescription> MotorwayJunctionDescriptionRef;

    /**
     * \ingroup Routing
     * Destination of the route
     */
    class OSMSCOUT_API DestinationDescription : public Description
    {
    private:
      std::string description;

    public:
      DestinationDescription(const std::string& description);

      std::string GetDebugString() const;

      std::string GetDescription() const;
    };

    typedef std::shared_ptr<DestinationDescription> DestinationDescriptionRef;

    /**
     * \ingroup Routing
     * A motorway junction
     */
    class OSMSCOUT_API MaxSpeedDescription : public RouteDescription::Description
    {
    private:
      uint8_t maxSpeed;

    public:
      MaxSpeedDescription(uint8_t speed);

      std::string GetDebugString() const;

      inline uint8_t GetMaxSpeed() const
      {
        return maxSpeed;
      }
    };

    typedef std::shared_ptr<MaxSpeedDescription> MaxSpeedDescriptionRef;

    /**
     * \ingroup Routing
     * Something has a type name. This is the name of the type of the way used.
     */
    class OSMSCOUT_API TypeNameDescription : public Description
    {
    private:
      std::string name;

    public:
      TypeNameDescription(const std::string& name);

      std::string GetDebugString() const;

      bool HasName() const;

      std::string GetName() const;

      std::string GetDescription() const;
    };

    typedef std::shared_ptr<TypeNameDescription> TypeNameDescriptionRef;

    /**
     * \ingroup Routing
     */
    class OSMSCOUT_API Node
    {
    private:
      DatabaseId                                     database;
      size_t                                         currentNodeIndex;
      std::vector<ObjectFileRef>                     objects;
      ObjectFileRef                                  pathObject;
      size_t                                         targetNodeIndex;
      double                                         distance;
      double                                         time;
      GeoCoord                                       location;
      std::unordered_map<std::string,DescriptionRef> descriptionMap;
      std::list<DescriptionRef>                      descriptions;

    public:
      Node(DatabaseId database,
           size_t currentNodeIndex,
           const std::vector<ObjectFileRef>& objects,
           const ObjectFileRef& pathObject,
           size_t targetNodeIndex);

      inline size_t GetCurrentNodeIndex() const
      {
        return currentNodeIndex;
      }

      /**
       * Return the objects that intersect at the current node index.
       */
      inline const std::vector<ObjectFileRef>& GetObjects() const
      {
        return objects;
      }

      /**
       * Return a list of descriptions attached to the current node
       */
      inline const std::list<DescriptionRef>& GetDescriptions() const
      {
        return descriptions;
      }

      /**
       * There exists a object/path from the current node to the next node
       * in the route.
       */
      inline bool HasPathObject() const
      {
        return pathObject.Valid();
      }

      inline DatabaseId GetDatabaseId() const
      {
        return database;
      }

      inline DBFileOffset GetDBFileOffset() const
      {
        return DBFileOffset(GetDatabaseId(),GetPathObject().GetFileOffset());
      }

      /**
       * Return the path object that connects the current node to the next node.
       */
      inline ObjectFileRef GetPathObject() const
      {
        return pathObject;
      }

      /**
       * The the index of the target node on the path that is the next node on the route.
       */
      inline size_t GetTargetNodeIndex() const
      {
        return targetNodeIndex;
      }

      /**
       * Distance from the start of the route in km.
       */
      inline double GetDistance() const
      {
        return distance;
      }

      /**
       * Time from the start of the route in h.
       */
      inline double GetTime() const
      {
        return time;
      }

      /**
       * Location (latitude,longitude) of the node
       */
      inline GeoCoord GetLocation() const
      {
        return location;
      }

      bool HasDescription(const char* name) const;
      DescriptionRef GetDescription(const char* name) const;

      void SetDistance(double distance);
      void SetTime(double time);
      void SetLocation(const GeoCoord &coord);

      void AddDescription(const char* name,
                          const DescriptionRef& description);
    };

  private:
    std::list<Node> nodes;

  public:
    RouteDescription();
    virtual ~RouteDescription();

    void Clear();

    void AddNode(DatabaseId database,
                 size_t currentNodeIndex,
                 const std::vector<ObjectFileRef>& objects,
                 const ObjectFileRef& pathObject,
                 size_t targetNodeIndex);

    inline std::list<Node>& Nodes()
    {
      return nodes;
    }

    inline const std::list<Node>& Nodes() const
    {
      return nodes;
    }
  };
}

#endif
